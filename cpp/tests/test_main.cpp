#include "pusch_lab.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
using namespace pusch;
namespace {
unsigned checks=0;
void expect(bool ok,const std::string& why) {++checks;if(!ok) throw std::runtime_error(why);}
void near(double a,double b,double eps,const std::string& why) {expect(std::abs(a-b)<=eps,why);}
void rejects(const std::function<void()>& f) {bool threw=false;try {f();}catch(const std::invalid_argument&){threw=true;}expect(threw,"input must be rejected");}
// Independent polynomial long division: MSB-first, append width zeroes.
unsigned crc_division(const Bits& bits,unsigned width,unsigned poly) {
    Bits work=bits;work.resize(bits.size()+width,0);
    for(std::size_t i=0;i<bits.size();++i) if(work[i]) {
        work[i]^=1;
        for(unsigned j=1;j<=width;++j) work[i+j]^=static_cast<Bit>((poly>>(width-j))&1U);
    }
    unsigned r=0;for(std::size_t i=bits.size();i<work.size();++i) r=(r<<1)|work[i];return r;
}
// Independent packed-state recurrence for Gold: bit 0 is current x(n).
Bits packed_gold(unsigned init,std::size_t n) {
    unsigned a=1,b=init;Bits out;
    for(std::size_t k=0;k<1600+n;++k) {
        if(k>=1600) out.push_back(static_cast<Bit>((a^b)&1U));
        const unsigned fa=((a>>3)^a)&1U,fb=((b>>3)^(b>>2)^(b>>1)^b)&1U;
        a=(a>>1)|(fa<<30);b=(b>>1)|(fb<<30);
    }
    return out;
}
}
int main() {
    try {
        expect(riv_encode(50,5,10)==455,"known RIV");expect(sliv_encode(0,12)==55,"known SLIV");
        for(unsigned n:{1U,2U,5U,50U}) for(unsigned l=1;l<=n;++l) for(unsigned s=0;s+l<=n;++s)
            expect(riv_decode(n,riv_encode(n,s,l))==std::make_pair(s,l),"RIV roundtrip");
        for(unsigned l=1;l<=14;++l) for(unsigned s=0;s+l<=14;++s)
            expect(sliv_decode(sliv_encode(s,l))==std::make_pair(s,l),"SLIV roundtrip");
        Config c;auto budget=resource_budget(c);
        expect(budget.actual_data_re==1380 && budget.g==2760 && budget.tbs_re==1380,"resource example");
        expect(tbs_from_re(1380,{2,526})==1416,"known TBS example");
        expect(tbs_from_re(138,{2,526})==136,"known small TBS");
        expect(tbs_from_re(6210,{2,526})==6400,"known large TBS");
        expect(tbs_from_re(3824,{2,512})==3824,"TBS branch endpoint");
        expect(tbs_from_re(3825,{2,512})==3840,"TBS upper branch");
        // Ninfo=5000 -> quantized=4992, C=2, TBS=16*ceil(5016/16)-24=5000.
        expect(tbs_from_re(20000,{2,128})==5000,"low-rate multi-block TBS");
        c.n_symbols=14;expect(resource_budget(c).actual_data_re==1620 && resource_budget(c).tbs_re==1560,"actual RE versus TBS cap");
        c.x_overhead=18;expect(resource_budget(c).actual_data_re==1620 && resource_budget(c).tbs_re==1440,"xOverhead is not grid puncturing");c=Config{};
        const auto bytes=bytes_to_bits({'1','2','3','4','5','6','7','8','9'});
        expect(crc_remainder(bytes,Crc::CRC16)==0x31c3,"CRC16 XMODEM known vector");
        for(const auto type:{Crc::CRC16,Crc::CRC24A,Crc::CRC24B}) {
            const unsigned width=type==Crc::CRC16 ? 16 : 24;
            const unsigned poly=type==Crc::CRC16 ? 0x1021 : type==Crc::CRC24A ? 0x864cfb : 0x800063;
            expect(crc_remainder(bytes,type)==crc_division(bytes,width,poly),"CRC independent division");
            auto attached=attach_crc(bytes,type);expect(check_crc(attached,type),"CRC appended remainder");
            attached[3]^=1;expect(!check_crc(attached,type),"single-bit CRC error");
        }
        const auto p=ldpc_plan(1416,526);
        expect(p.bg==2 && p.tb_crc==16 && p.b==1432 && p.c==1 && p.zc==144 && p.i_ls==4 && p.k==1440 && p.n==7200 && p.filler==8,"LDPC known plan");
        const auto multi=ldpc_plan(16000,512);
        expect(multi.bg==1 && multi.c==2 && multi.cb_crc==24 && multi.k_prime==8036 && multi.zc==384,"segmented LDPC plan");
        const auto segments=segment_tb(attach_crc(Bits(16000,1),Crc::CRC24A),multi);
        for(const auto& cb:segments) expect(check_crc(Bits(cb.begin(),cb.begin()+multi.k_prime),Crc::CRC24B),"each CB CRC");
        const auto e=cb_output_lengths(20,2,1,3);expect(e==std::vector<unsigned>({6,6,8}),"E distribution");
        const std::vector<std::int8_t> fake(50*2,0);
        const auto interleaved=rate_match(fake,2,2,0,8,2);
        expect(interleaved.source_positions==std::vector<unsigned>({0,4,1,5,2,6,3,7}),"known bit interleave");
        auto erased=fake;erased[0]=-1;const auto skipped=rate_match(erased,2,2,0,8,2);
        expect(skipped.source_positions[0]==1,"skip NULL");
        const auto repetition=rate_match(fake,2,2,0,202,2);
        const auto recovered=rate_recover(std::vector<double>(202,1),repetition.source_positions,100);
        expect(recovered[0]==3 && recovered[1]==3 && recovered[2]==2,"LLR repetition summing");
        for(unsigned rv=0;rv<4;++rv) {
            const auto rm=rate_match(fake,2,2,rv,8,2);
            expect(rm.source_positions[0]==std::vector<unsigned>({0,26,50,86})[rv],"RV k0");
        }
        for(unsigned init:{0U,1U,42U,0x7fffffffU}) expect(gold(init,257)==packed_gold(init,257),"Gold independent packed implementation");
        expect(scramble_data(scramble_data(bytes,c.rnti,c.data_nid),c.rnti,c.data_nid)==bytes,"scramble involution");
        for(unsigned qm:{2U,4U}) {
            Bits labels;for(unsigned v=0;v<(1U<<qm);++v) for(unsigned bit=0;bit<qm;++bit) labels.push_back(static_cast<Bit>((v>>(qm-1-bit))&1U));
            const auto symbols=qam_modulate(labels,qm);expect(qam_hard_demodulate(symbols,qm)==labels,"all QAM labels");
            double energy=0;for(const auto& z:symbols) energy+=std::norm(z);
            near(energy/symbols.size(),1,1e-14,"unit constellation average energy");
            const auto llr=qam_maxlog_llr(symbols,qm,.1);
            for(std::size_t i=0;i<labels.size();++i) expect((llr[i]<0)==bool(labels[i]),"LLR positive-zero convention");
        }
        const auto pilots=dmrs_indices(c),data=data_indices(c);
        expect(pilots.size()==60 && data.size()==1380,"grid counts");
        std::set<std::size_t> pilotset(pilots.begin(),pilots.end());
        for(auto i:data) expect(pilotset.count(i)==0,"disjoint pilot/data maps");
        Config same=c;same.crb_start=5;same.start_prb=0;
        expect(dmrs_type1(c)==dmrs_type1(same),"DMRS uses CRB reference");
        const std::uint64_t init=(1ULL<<17)*(14ULL*c.n_slot+c.dmrs_symbol+1)*(2ULL*c.dmrs_nid+1)+2ULL*c.dmrs_nid;
        const auto dmrsbits=packed_gold(static_cast<unsigned>(init&0x7fffffff),180);
        expect(dmrs_type1(c)==qam_modulate(Bits(dmrsbits.begin()+60,dmrsbits.end()),2),"DMRS init/offset independent Gold");
        std::vector<IQ> values={{1,2},{3,4},{-1,1},{2,-3}},direct(4);
        for(unsigned k=0;k<4;++k) for(unsigned n=0;n<4;++n) direct[k]+=values[n]*std::polar(.5,-2*3.14159265358979323846*k*n/4);
        auto transformed=values;fft_inplace(transformed,false);
        for(unsigned k=0;k<4;++k) near(std::abs(transformed[k]-direct[k]),0,1e-13,"FFT versus direct DFT");
        fft_inplace(transformed,true);for(unsigned k=0;k<4;++k) near(std::abs(transformed[k]-values[k]),0,1e-13,"FFT inverse");
        Bits coded(2760);for(std::size_t i=0;i<coded.size();++i) coded[i]=static_cast<Bit>((i/7+i)%2);
        const auto symbols=qam_modulate(scramble_data(coded,c.rnti,c.data_nid),2);
        const auto grid=map_grid(c,symbols);const auto tx=ofdm_modulate(grid,c.bwp_prbs);
        expect(tx.size()==14*(1024+72),"sample count");
        const IQ h(.8,.3);const auto rx=ofdm_demodulate(flat_awgn(tx,h,0,42),c.bwp_prbs);
        near(std::abs(estimate_flat_channel(c,rx)-h),0,1e-12,"LS channel exact noiseless");
        auto equalized=extract_data(c,rx);for(auto& z:equalized) z/=h;
        near(evm_rms(symbols,equalized),0,1e-11,"noiseless OFDM EVM");
        expect(scramble_data(qam_hard_demodulate(equalized,2),c.rnti,c.data_nid)==coded,"noiseless end-to-end bit recovery");
        const std::vector<Tap> taps={{0,{1,0}},{2,{0,.5}}};std::vector<IQ> impulse(5);impulse[0]=1;
        const auto channel=tapped_channel(impulse,taps,0,100,0,1);
        expect(channel[0]==IQ(1,0) && channel[2]==IQ(0,.5) && channel[1]==IQ{},"FIR impulse response");
        near(rms_delay_seconds(taps,100),.008,1e-15,"power weighted RMS delay");
        const auto cfo=tapped_channel(std::vector<IQ>(4,IQ(1,0)),{{0,{1,0}}},25,100,0,1);
        near(std::abs(cfo[1]-IQ(0,1)),0,1e-15,"CFO phase rotation");
        near(linear_mean_dbm({-80,-100}),-82.96708622,1e-8,"linear power averaging");
        near(pusch_power_dbm(23,-80,1,10,1,90,0,0),23,1e-12,"power clipping");
        rejects([]{mcs_table1(29);});rejects([]{riv_encode(50,49,2);});rejects([]{sliv_encode(13,2);});
        rejects([]{qam_modulate({2,0},2);});rejects([]{qam_maxlog_llr({{1,1}},2,0);});
        rejects([]{std::vector<IQ> bad(3);fft_inplace(bad,false);});
        rejects([]{rate_match(std::vector<std::int8_t>(100,-1),2,2,0,8,2);});
        std::cout<<"PASS "<<checks<<" checks (educational scope; not 3GPP conformance certification)\n";
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
