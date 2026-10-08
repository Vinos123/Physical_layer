#include "pusch_lab.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
// Monte Carlo of modulation only: perfect channel h=1, exact noise variance.
// Baseline for debugging QAM/LLR, not a PUSCH BLER curve.
int main(int argc,char** argv) {
    try {
        const unsigned qm=argc>1 ? static_cast<unsigned>(std::stoul(argv[1])) : 2;
        const unsigned frames=argc>2 ? static_cast<unsigned>(std::stoul(argv[2])) : 100;
        if((qm!=2 && qm!=4) || frames<1 || frames>100000) throw std::invalid_argument("usage: pusch_sweep [2|4] [frames:1..100000]");
        std::cout<<"snr_data_esn0_db,qm,bits,errors,ber,evm_rms\n"<<std::setprecision(9);
        for(int snr=0;snr<=20;snr+=2) {
            std::mt19937 rng(2026U+static_cast<unsigned>(snr));
            std::uint64_t errors=0,bits=0;double errenergy=0,refenergy=0;
            const double variance=std::pow(10.0,-double(snr)/10);
            for(unsigned frame=0;frame<frames;++frame) {
                pusch::Bits input(1380U*qm);for(auto& b:input) b=static_cast<pusch::Bit>(rng()&1U);
                const auto symbols=pusch::qam_modulate(input,qm);
                const auto rx=pusch::flat_awgn(symbols,{1,0},variance,rng());
                const auto hard=pusch::qam_hard_demodulate(rx,qm);
                for(std::size_t i=0;i<input.size();++i) errors+=(hard[i]!=input[i]);
                for(std::size_t i=0;i<symbols.size();++i) {errenergy+=std::norm(rx[i]-symbols[i]);refenergy+=std::norm(symbols[i]);}
                bits+=input.size();
            }
            std::cout<<snr<<","<<qm<<","<<bits<<","<<errors<<","<<double(errors)/double(bits)<<","<<std::sqrt(errenergy/refenergy)<<"\n";
        }
    } catch(const std::exception& e) {std::cerr<<e.what()<<"\n";return 1;}
}
