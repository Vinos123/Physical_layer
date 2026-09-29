#include "nr_core.hpp"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <tuple>

void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void rejects(F f){bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}check(caught,"missing input validation");}
int main(){
 try{
    for(auto [n,r,t]:{std::tuple<double,double,int>{24,.5,24},{3824,.5,3824},{3825,.5,3840},
                     {4000,.5,3968},{10000,.5,9992},{10000,.2,9984},{4184,.5,4224}})
        check(nr::tbs_from_ninfo(n,r)==t,"TBS boundary/rounding");
    auto b=nr::resource_budget();check(b.tbs==4992&&b.g==10560,"main TBS/G");
    b=nr::resource_budget(4,12,12,0,2,120./1024);check(b.tbs==120,"small TBS");
    b=nr::resource_budget(20,14,6);check(b.re_tbs==3120&&b.re_actual==3240,"RE cap distinction");
    b=nr::resource_budget(20,12,12,6);check(b.re_actual==-1&&b.g==-1,"overhead is not actual map");
    std::cout<<"PASS TBS branches, half ties, RE accounting\n";
    auto d=nr::ldpc_sizes(4992,490./1024);
    check(d.bg==1&&d.zc==240&&d.k==5280&&d.filler==264,"main LDPC sizes");
    d=nr::ldpc_sizes(120,120./1024);check(d.bg==2&&d.kb==6&&d.zc==24&&d.k==240,"small BG2 lifting");
    d=nr::ldpc_sizes(9992,.5);check(d.c==2&&d.cb_crc==24&&d.kprime==5032,"segmentation");
    check(nr::ldpc_sizes(3824,.67).bg==2&&nr::ldpc_sizes(3824,.671).bg==1,"BG boundary .67");
    check(nr::ldpc_sizes(3840,.25).bg==2&&nr::ldpc_sizes(3840,.251).bg==1,"BG boundary .25");
    check(nr::ldpc_sizes(288,.9).bg==2&&nr::ldpc_sizes(296,.9).bg==1,"BG small A boundary");
    std::cout<<"PASS CRC/BG/segmentation/lifting sizes\n";
    check(nr::riv(52,10,20)==998&&nr::sliv(2,12)==53,"allocation golden values");
    for(int n:{14,52,275}){
        std::set<int> values;
        for(int s=0;s<n;++s)for(int l=1;l<=n-s;++l)values.insert(nr::riv(n,s,l));
        check(values.size()==std::size_t(n*(n+1)/2),"RIV uniqueness");
    }
    std::set<int> slivs;for(int s=0;s<14;++s)for(int l=1;l<=14-s;++l)slivs.insert(nr::sliv(s,l));
    check(slivs.size()==105,"SLIV uniqueness");std::cout<<"PASS exhaustive mathematical interval encoding\n";
    // Independent integer-register reference versus array recurrences.
    for(std::uint32_t seed:{0u,1u,42u,0x1234u*(1u<<15)+42,0x7fffffffu}){
        auto actual=nr::gold(seed,256);std::uint32_t a=1,bits=seed;
        for(int i=0;i<1856;++i){
            if(i>=1600)check(actual[i-1600]==((a^bits)&1u),"Gold reference mismatch");
            const auto aa=((a>>3)^a)&1u,bb=((bits>>3)^(bits>>2)^(bits>>1)^bits)&1u;
            a=(a>>1)|(aa<<30);bits=(bits>>1)|(bb<<30);
        }
    }
    std::cout<<"PASS Gold against independent register implementation\n";
    const nr::Bits bits={0,0,0,1,1,0,1,1};const auto q=nr::qpsk(bits);
    const nr::Symbols expected={{1,1},{1,-1},{-1,1},{-1,-1}};
    for(int i=0;i<4;++i)check(std::abs(q[i]-expected[i]/std::sqrt(2.))<1e-14,"QPSK mapping");
    check(nr::hard_qpsk(q)==bits,"QPSK roundtrip");
    for(auto z:nr::dmrs_cp(0,2,120))check(std::abs(std::abs(z)-1)<1e-14,"DMRS energy");
    std::cout<<"PASS QPSK known constellation and DMRS energy\n";
    for(int n:{1,2,8,64}){
        nr::Symbols x;for(int i=0;i<n;++i)x.emplace_back(std::sin(.37*i),std::cos(.29*i));
        const auto a=nr::fft(x),reference=nr::dft(x),roundtrip=nr::fft(a,true);
        double ein=0,eout=0;
        for(int i=0;i<n;++i){
            check(std::abs(a[i]-reference[i])<1e-11,"FFT versus direct DFT");
            check(std::abs(roundtrip[i]-x[i])<1e-11,"FFT inverse");
            ein+=std::norm(x[i]);eout+=std::norm(a[i]);
        }
        check(std::abs(ein-eout)<1e-10,"Parseval");
    }
    std::cout<<"PASS independent DFT/FFT and energy checks\n";
    const auto [requested,actual]=nr::pusch_power(160,1,-80,.8,100);
    check(std::abs(requested-25.0514997832)<1e-9&&actual==23,"power saturation");
    rejects([]{nr::sliv(12,3);});rejects([]{nr::riv(52,50,3);});
    rejects([]{nr::tbs_from_ninfo(0,.5);});rejects([]{nr::resource_budget(20,1,12);});
    rejects([]{nr::ldpc_sizes(7,.5);});rejects([]{nr::gold(1u<<31,3);});
    rejects([]{nr::fft(nr::Symbols(3));});rejects([]{nr::qpsk({0,2});});
    std::cout<<"PASS power clipping and invalid inputs\nAll 7 check groups passed.\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
