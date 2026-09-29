#include "nr_core.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs=std::filesystem;
using nr::Bits;using nr::Symbols;using nr::Complex;
Bits random_bits(std::mt19937& gen,int count){
    Bits b(static_cast<std::size_t>(count));for(auto& x:b)x=gen()&1u;return b;
}
void ex01(std::ostream& out,const fs::path&){
    out<<"SCS_kHz=30 slot_ms=0.5 slots_per_frame=20 allocated_width_MHz=7.2\n"
       <<"RIV="<<nr::riv(52,10,20)<<" SLIV="<<nr::sliv(2,12)
       <<" grant_slot=4 K2=2 pusch_slot=6 (same SCS)\n";
}
void budget_line(std::ostream& out,const std::string& label,const nr::Budget& b){
    out<<label<<": NRE_TBS="<<b.re_tbs<<" NRE_actual="<<b.re_actual<<" Ninfo="<<b.ninfo
       <<" TBS="<<b.tbs<<" G="<<b.g<<'\n';
}
void ex02(std::ostream& out,const fs::path&){
    const auto b=nr::resource_budget();budget_line(out,"main",b);
    const auto d=nr::ldpc_sizes(b.tbs,490./1024);
    out<<"TB_CRC="<<d.tb_crc<<" B="<<d.b<<" BG="<<d.bg<<" C="<<d.c<<" CB_CRC="<<d.cb_crc
       <<" Kprime="<<d.kprime<<" Kb="<<d.kb<<" Zc="<<d.zc<<" K="<<d.k
       <<" filler="<<d.filler<<" N="<<d.n<<'\n';
    budget_line(out,"small",nr::resource_budget(4,12,12,0,2,120./1024));
    budget_line(out,"RE_cap",nr::resource_budget(20,14,6));
    out<<"ideal_TB_Mbps="<<b.tbs/.0005/1e6<<'\n';
}
void ex03(std::ostream& out,const fs::path& path){
    // l=2..13, PRBs 0..19, CRB origin 0; DMRS type1 port0 at l=2,11.
    std::vector<int> mask(14*240);
    int data_count=0,pilot_count=0;
    for(int l=2;l<14;++l)for(int k=0;k<240;++k){
        const bool pilot=(l==2||l==11)&&k%2==0;
        mask[l*240+k]=pilot?2:1;pilot?++pilot_count:++data_count;
    }
    std::mt19937 gen(20260929);
    const auto bits=random_bits(gen,2*data_count),seq=nr::gold(0x1234u*(1u<<15)+42,2*data_count);
    Bits scrambled=bits;for(std::size_t i=0;i<bits.size();++i)scrambled[i]^=seq[i];
    const auto symbols=nr::qpsk(scrambled);Symbols grid(mask.size());
    std::size_t at=0;
    for(std::size_t i=0;i<mask.size();++i)if(mask[i]==1)grid[i]=symbols[at++];
    for(int l:{2,11}){
        const auto pilots=nr::dmrs_cp(0,l,120);
        for(int i=0;i<120;++i)grid[l*240+2*i]=pilots[i];
    }
    Symbols recovered_symbols;
    for(std::size_t i=0;i<mask.size();++i)if(mask[i]==1)recovered_symbols.push_back(grid[i]);
    auto recovered=nr::hard_qpsk(recovered_symbols);int errors=0;
    for(std::size_t i=0;i<bits.size();++i)errors+=((recovered[i]^seq[i])!=bits[i]);
    std::ofstream csv(path/"grid_mask.csv");csv.exceptions(std::ios::failbit|std::ios::badbit);
    for(int l=0;l<14;++l){for(int k=0;k<240;++k)csv<<(k?",":"")<<mask[l*240+k];csv<<'\n';}
    std::ofstream pilot_csv(path/"dmrs.csv");pilot_csv.exceptions(std::ios::failbit|std::ios::badbit);
    pilot_csv<<"symbol,subcarrier,real,imag\n"<<std::setprecision(17);
    for(int l:{2,11})for(int k=0;k<240;k+=2)
        pilot_csv<<l<<','<<k<<','<<grid[l*240+k].real()<<','<<grid[l*240+k].imag()<<'\n';
    out<<"data_RE="<<data_count<<" DMRS_RE="<<pilot_count<<" QPSK_bits="<<bits.size()
       <<" bit_errors="<<errors<<" (mapping only, no UL-SCH codec)\n";
    if(errors!=0)throw std::runtime_error("mapping roundtrip failed");
}
void ex04(std::ostream& out,const fs::path& path){
    // Fixed CP and no reference signals: waveform principle experiment only.
    constexpr int n=256,m=72,cp=32,trials=400;
    std::mt19937 gen(44);std::vector<double> papr[2];double maxerr[2]={};
    std::ofstream csv(path/"papr.csv");csv.exceptions(std::ios::failbit|std::ios::badbit);
    csv<<"waveform,trial,PAPR_dB\n";
    for(int t=0;t<trials;++t){
        const auto data=nr::qpsk(random_bits(gen,2*m));
        for(int mode=0;mode<2;++mode){
            const auto spread=mode?nr::dft(data):data;
            Symbols grid(n);std::copy(spread.begin(),spread.end(),grid.begin()+20);
            const auto x=nr::fft(grid,true);
            Symbols with_cp(x.end()-cp,x.end());with_cp.insert(with_cp.end(),x.begin(),x.end());
            const auto rx=nr::fft(Symbols(with_cp.begin()+cp,with_cp.end()));
            Symbols recovered(rx.begin()+20,rx.begin()+20+m);
            if(mode)recovered=nr::dft(recovered,true);
            for(int i=0;i<m;++i)maxerr[mode]=std::max(maxerr[mode],std::abs(recovered[i]-data[i]));
            Symbols oversampled(4*n);std::copy(spread.begin(),spread.end(),oversampled.begin()+20);
            const double p=nr::papr_db(nr::fft(oversampled,true));papr[mode].push_back(p);
            csv<<(mode?"DFT_s_OFDM":"CP_OFDM")<<','<<t<<','<<p<<'\n';
        }
    }
    for(int mode=0;mode<2;++mode){
        auto values=papr[mode];std::sort(values.begin(),values.end());
        out<<(mode?"DFT_s_OFDM":"CP_OFDM")<<": roundtrip_error="<<maxerr[mode]
           <<" mean_PAPR_dB="<<std::accumulate(values.begin(),values.end(),0.)/trials
           <<" p99_nearest_rank_dB="<<values[395]<<'\n';
        if(maxerr[mode]>1e-10)throw std::runtime_error("OFDM roundtrip failed");
    }
}
void ex05(std::ostream& out,const fs::path& path){
    std::mt19937 gen(55);std::normal_distribution<double> normal;
    const auto bits=random_bits(gen,200000);const auto data=nr::qpsk(bits);
    const Complex h(.7,.4);const auto pilot=nr::dmrs_cp(0,2,120);
    std::ofstream csv(path/"estimation.csv");csv.exceptions(std::ios::failbit|std::ios::badbit);
    csv<<"EsN0_dB,BER,EVM_pct,channel_est_error\n";
    for(int snr:{0,5,10,20}){
        const double n0=std::pow(10.,-snr/10.);
        auto noise=[&](){const double re=normal(gen),im=normal(gen);return std::sqrt(n0/2)*Complex(re,im);};
        Complex hhat=0;for(auto p:pilot)hhat+=(h*p+noise())/p;hhat/=double(pilot.size());
        Symbols z;double error_energy=0;
        for(auto x:data){auto estimate=(h*x+noise())/hhat;z.push_back(estimate);error_energy+=std::norm(estimate-x);}
        const auto got=nr::hard_qpsk(z);int errors=0;
        for(std::size_t i=0;i<bits.size();++i)errors+=bits[i]!=got[i];
        const double ber=double(errors)/double(bits.size()),evm=100*std::sqrt(error_energy/double(data.size()));
        out<<"EsN0_dB="<<snr<<" BER="<<ber<<" EVM_pct="<<evm<<" channel_error="<<std::abs(hhat-h)<<'\n';
        csv<<snr<<','<<ber<<','<<evm<<','<<std::abs(hhat-h)<<'\n';
    }
}
void ex06(std::ostream& out,const fs::path&){
    std::mt19937 gen(66);std::normal_distribution<double> normal;
    const auto bits=random_bits(gen,200000);int first=0,combined=0;
    constexpr double variance=.5;
    for(auto b:bits){
        const double x=1.-2*b;
        const double y1=x+std::sqrt(variance)*normal(gen),y2=x+std::sqrt(variance)*normal(gen);
        const double l1=2*y1/variance,l2=2*y2/variance;
        first+=((l1<0)!=b);combined+=((l1+l2<0)!=b);
    }
    out<<"first_BER="<<double(first)/double(bits.size())<<" combined_BER="<<double(combined)/double(bits.size())
       <<" (same-symbol repetition; double total energy; no LDPC/RV/CRC)\n";
}
void ex07(std::ostream& out,const fs::path& path){
    std::ofstream csv(path/"power.csv");csv.exceptions(std::ios::failbit|std::ios::badbit);
    csv<<"PRBs,requested_dBm,actual_dBm,relative_PSD_dB\n";
    for(int prbs:{20,40,80,160}){
        const auto [requested,actual]=nr::pusch_power(prbs,1,-80,.8,100);
        const double relative=actual-10*std::log10(2.*prbs);
        out<<"PRBs="<<prbs<<" requested_dBm="<<requested<<" actual_dBm="<<actual
           <<" relative_PSD_dB="<<relative<<'\n';
        csv<<prbs<<','<<requested<<','<<actual<<','<<relative<<'\n';
    }
    out<<"illustrative_RSRQ_dB="<<10*std::log10(50.)-95+65
       <<" mean_power_dBm="<<10*std::log10((std::pow(10.,-8)+std::pow(10.,-10))/2)
       <<" mean_of_dBm=-90 (different quantity)\n";
}
int main(int argc,char** argv){
    try{
        int selected=0;fs::path path="results";
        for(int i=1;i<argc;++i){
            const std::string arg=argv[i];
            if(arg=="--out"&&i+1<argc)path=argv[++i];
            else if(arg=="--example"&&i+1<argc){
                const std::string value=argv[++i];std::size_t used=0;selected=std::stoi(value,&used);
                if(used!=value.size()||selected<1||selected>7)throw std::invalid_argument("example must be 1..7");
            }else throw std::invalid_argument("usage: nr_examples [--example 1..7] [--out directory]");
        }
        fs::create_directories(path);
        const auto filename=selected?"ex"+std::to_string(selected)+".txt":"all_results.txt";
        std::ofstream report(path/filename);report.exceptions(std::ios::failbit|std::ios::badbit);
        using Experiment=void(*)(std::ostream&,const fs::path&);
        const Experiment examples[]={ex01,ex02,ex03,ex04,ex05,ex06,ex07};
        for(int i=0;i<7;++i)if(selected==0||selected==i+1){
            std::ostringstream buffer;buffer<<std::setprecision(10)<<"[ex"<<i+1<<"]\n";
            examples[i](buffer,path);std::cout<<buffer.str();report<<buffer.str();
        }
        std::cout<<"Saved results to "<<fs::absolute(path)<<'\n';
    }catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<'\n';return 1;}
}
