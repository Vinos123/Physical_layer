#include "pusch_lab.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
using namespace pusch;
int main(int argc,char** argv) {
    try {
        const std::string mode=argc>1 ? argv[1] : "budget";
        std::cout<<std::setprecision(10);
        Config c;const auto b=resource_budget(c);
        if(mode=="budget") {
            const auto a=tbs_from_re(b.tbs_re,{c.qm,c.r_x1024},c.layers);
            const auto p=ldpc_plan(a,c.r_x1024);
            std::cout<<"RIV="<<riv_encode(c.bwp_prbs,c.start_prb,c.n_prbs)
                     <<" SLIV="<<sliv_encode(c.start_symbol,c.n_symbols)<<"\n"
                     <<"actual_data_RE="<<b.actual_data_re<<" TBS_RE="<<b.tbs_re<<" G="<<b.g<<" TBS="<<a<<"\n"
                     <<"TB_CRC="<<p.tb_crc<<" B="<<p.b<<" BG="<<p.bg<<" C="<<p.c
                     <<" Zc="<<p.zc<<" iLS="<<p.i_ls<<" K="<<p.k<<" N="<<p.n<<" filler="<<p.filler<<"\n";
        } else if(mode=="coding") {
            const auto p=ldpc_plan(1416,526);
            Bits tb(1416);for(std::size_t i=0;i<tb.size();++i) tb[i]=static_cast<Bit>((i*7+3)%2);
            const auto with_crc=attach_crc(tb,Crc::CRC16);
            const auto cb=segment_tb(with_crc,p);
            std::vector<std::int8_t> synthetic(p.n);
            for(unsigned i=0;i<p.n;++i) synthetic[i]=static_cast<std::int8_t>((i/7+i)%2);
            const auto mask=filler_mask_after_puncture(p);
            for(unsigned i=0;i<p.n;++i) if(mask[i]) synthetic[i]=-1;
            std::cout<<"CRC valid="<<check_crc(with_crc,Crc::CRC16)<<" K="<<cb[0].size()<<"\n"
                     <<"Synthetic rate-matcher input: NOT an LDPC codeword.\n";
            for(unsigned rv=0;rv<4;++rv) {
                const auto rm=rate_match(synthetic,p.bg,p.zc,rv,2760,2);
                std::cout<<"RV="<<rv<<" E="<<rm.bits.size()<<" first_source="<<rm.source_positions[0]<<"\n";
            }
        } else if(mode=="link") {
            const double snr=argc>2 ? std::stod(argv[2]) : 12.0;
            c.qm=argc>3 ? static_cast<unsigned>(std::stoul(argv[3])) : 2U;
            const auto seed=argc>4 ? static_cast<std::uint32_t>(std::stoul(argv[4])) : 2026U;
            if(!std::isfinite(snr) || snr< -40 || snr>120) throw std::invalid_argument("SNR must be -40..120 dB");
            const auto budget=resource_budget(c);Bits input(static_cast<std::size_t>(budget.g));
            std::mt19937 rng(seed);for(auto& bit:input) bit=static_cast<Bit>(rng()&1U);
            const auto scrambled=scramble_data(input,c.rnti,c.data_nid);
            const auto symbols=qam_modulate(scrambled,c.qm);
            const auto tx=ofdm_modulate(map_grid(c,symbols),c.bwp_prbs);
            const IQ h(0.8,0.3);const double noise=std::norm(h)*std::pow(10.0,-snr/10);
            const auto rx=ofdm_demodulate(flat_awgn(tx,h,noise,seed+1),c.bwp_prbs);
            const auto estimate=estimate_flat_channel(c,rx);
            if(std::abs(estimate)<1e-12) throw std::runtime_error("channel estimate too small");
            auto equalized=extract_data(c,rx);for(auto& z:equalized) z/=estimate;
            auto llr=qam_maxlog_llr(equalized,c.qm,noise/std::norm(estimate));
            const auto seq=gold((std::uint32_t(c.rnti)<<15)+c.data_nid,input.size());
            std::size_t errors=0;
            for(std::size_t i=0;i<input.size();++i) {if(seq[i]) llr[i]=-llr[i];errors+=((llr[i]<0)!=bool(input[i]));}
            std::cout<<"Uncoded modulation-stage link; no LDPC, UCI or HARQ.\n"
                     <<"SNR_data_EsN0_dB="<<snr<<" Qm="<<c.qm<<" bits="<<input.size()<<" errors="<<errors
                     <<" BER="<<double(errors)/double(input.size())<<"\n"
                     <<"h="<<h<<" h_hat="<<estimate<<" EVM_rms="<<evm_rms(symbols,equalized)<<"\n";
        } else if(mode=="measure") {
            for(unsigned prbs:{10U,20U}) {
                const auto requested=pusch_power_dbm(100,-80,1,prbs,1,90,0,0);
                const auto actual=pusch_power_dbm(23,-80,1,prbs,1,90,0,0);
                std::cout<<"PRB="<<prbs<<" requested_dBm="<<requested<<" capped_dBm="<<actual<<"\n";
            }
            std::cout<<"linear_mean_dBm="<<linear_mean_dbm({-80,-100})<<"\n"
                     <<"RSRQ_example_dB="<<10*std::log10(50.0)-90+60<<"\n";
        } else if(mode=="channel") {
            const std::vector<Tap> taps={{0,IQ(std::sqrt(0.7),0)},{3,IQ(0,std::sqrt(0.2))},{8,IQ(std::sqrt(0.1),0)}};
            std::vector<IQ> impulse(12);impulse[0]=IQ(1,0);
            const auto out=tapped_channel(impulse,taps,0,30.72e6,0,1);
            std::cout<<"Static example FIR (not 3GPP TDL); RMS_delay_ns="<<rms_delay_seconds(taps,30.72e6)*1e9<<"\n";
            for(std::size_t i=0;i<out.size();++i) std::cout<<i<<","<<out[i].real()<<","<<out[i].imag()<<"\n";
        } else throw std::invalid_argument("usage: pusch_demo budget|coding|link [EsN0_dB] [2|4] [seed]|measure|channel");
    } catch(const std::exception& e) {std::cerr<<e.what()<<"\n";return 1;}
}
