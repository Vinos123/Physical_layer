#include "nr_core.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace nr {
namespace {
constexpr double pi=3.14159265358979323846;
// Numerical TBS entries from TS 38.214 V18.5.0, table 5.1.3.2-1.
const std::vector<int> small_tbs={
 24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,
 168,176,184,192,208,224,240,256,272,288,304,320,336,352,368,384,
 408,432,456,480,504,528,552,576,608,640,672,704,736,768,808,848,
 888,928,984,1032,1064,1128,1160,1192,1224,1256,1288,1320,1352,
 1416,1480,1544,1608,1672,1736,1800,1864,1928,2024,2088,2152,
 2216,2280,2408,2472,2536,2600,2664,2728,2792,2856,2976,3104,
 3240,3368,3496,3624,3752,3824};
void require(bool ok,const char* why){if(!ok)throw std::invalid_argument(why);}
int div_up(int a,int b){return (a+b-1)/b;}
}

int tbs_from_ninfo(double ninfo,double rate){
    require(std::isfinite(ninfo)&&ninfo>0&&ninfo<=2000000&&rate>0&&rate<1,
            "Ninfo outside teaching range or invalid rate");
    if(ninfo<=3824){
        const int n=std::max(3,static_cast<int>(std::floor(std::log2(ninfo)))-6);
        const double step=std::pow(2.,n);
        const int q=std::max(24,static_cast<int>(step*std::floor(ninfo/step)));
        return *std::lower_bound(small_tbs.begin(),small_tbs.end(),q);
    }
    const int n=static_cast<int>(std::floor(std::log2(ninfo-24)))-5;
    const double step=std::pow(2.,n);
    // Positive half ties round upwards, explicitly matching the standard.
    const int q=std::max(3840,static_cast<int>(step*std::floor((ninfo-24)/step+0.5)));
    const int c=rate<=.25?div_up(q+24,3816):(q>8424?div_up(q+24,8424):1);
    return 8*c*div_up(q+24,8*c)-24;
}

Budget resource_budget(int nprb,int symbols,int dmrs_re,int overhead,int qm,double rate,int layers){
    require(nprb>=1&&nprb<=275&&symbols>=1&&symbols<=14&&layers>=1&&layers<=4,
            "outside ordinary teaching profile");
    require((qm==2||qm==4||qm==6||qm==8)&&rate>0&&rate<1,"invalid modulation/rate");
    require(dmrs_re>=0&&overhead>=0&&dmrs_re<=168&&overhead<=168&&
            dmrs_re+overhead<12*symbols,"invalid RE overhead");
    const int per_rb=12*symbols-dmrs_re-overhead;
    const int ntbs=std::min(156,per_rb)*nprb;
    const double ni=ntbs*qm*rate*layers;
    const int actual=overhead==0?nprb*(12*symbols-dmrs_re):-1;
    return {ntbs,actual,tbs_from_ninfo(ni,rate),actual<0?-1:actual*qm*layers,ni};
}

LDPCSizes ldpc_sizes(int a,double rate){
    require(a>0&&a<=2000000&&a%8==0&&rate>0&&rate<1,"invalid TB/rate");
    const int bg=(a<=292||(a<=3824&&rate<=.67)||rate<=.25)?2:1;
    const int crc=a<=3824?16:24,b=a+crc,kcb=bg==1?8448:3840;
    const int c=b<=kcb?1:div_up(b,kcb-24),cb_crc=c==1?0:24;
    require((b+c*cb_crc)%c==0,"use standard TBS with integral Kprime");
    const int kp=(b+c*cb_crc)/c;
    const int kb=bg==1?22:b>640?10:b>560?9:b>192?8:6;
    int zc=10000;
    for(int base:{2,3,5,7,9,11,13,15})
        for(int z=base;z<=384;z*=2)if(kb*z>=kp)zc=std::min(zc,z);
    require(zc<=384,"lifting size not available");
    const int k=(bg==1?22:10)*zc;
    return {a,crc,b,bg,c,cb_crc,kp,kb,zc,k,k-kp,(bg==1?66:50)*zc};
}

int riv(int n,int start,int length){
    require(n>0&&n<=275&&start>=0&&start<n&&length>=1&&length<=n-start,
            "invalid RB interval");
    return length-1<=n/2?n*(length-1)+start:n*(n-length+1)+(n-1-start);
}
int sliv(int start,int length){
    require(start>=0&&start<14&&length>=1&&length<=14-start,"invalid symbol interval");
    return length-1<=7?14*(length-1)+start:14*(15-length)+13-start;
}

Bits gold(std::uint32_t cinit,int count){
    require(cinit<(1u<<31)&&count>=0&&count<=20000000,"invalid Gold input");
    constexpr int nc=1600;
    Bits x1(static_cast<std::size_t>(nc+count+31)),x2(x1.size()),out(static_cast<std::size_t>(count));
    x1[0]=1;
    for(int i=0;i<31;++i)x2[i]=static_cast<std::uint8_t>((cinit>>i)&1u);
    for(int i=0;i<nc+count;++i){
        x1[i+31]=x1[i+3]^x1[i];
        x2[i+31]=x2[i+3]^x2[i+2]^x2[i+1]^x2[i];
    }
    for(int i=0;i<count;++i)out[i]=x1[nc+i]^x2[nc+i];
    return out;
}
Symbols qpsk(const Bits& bits){
    require(bits.size()%2==0,"QPSK needs bit pairs");
    Symbols out;out.reserve(bits.size()/2);
    for(std::size_t i=0;i<bits.size();i+=2){
        require(bits[i]<=1&&bits[i+1]<=1,"nonbinary input");
        out.emplace_back((1.-2*bits[i])/std::sqrt(2.),(1.-2*bits[i+1])/std::sqrt(2.));
    }
    return out;
}
Bits hard_qpsk(const Symbols& symbols){
    Bits out;out.reserve(2*symbols.size());
    for(auto z:symbols){out.push_back(z.real()<0);out.push_back(z.imag()<0);}
    return out;
}
Symbols dmrs_cp(int slot,int symbol,int count,int nid,int nscid){
    // Normal CP and basic CP-OFDM DMRS; sequence origin is CRB 0.
    require(slot>=0&&slot<160&&symbol>=0&&symbol<14&&count>=0&&count<=1000000&&
            nid>=0&&nid<=65535&&(nscid==0||nscid==1),"invalid DMRS input");
    const auto ci=((1ULL<<17)*(14ULL*slot+symbol+1)*(2ULL*nid+1)+2ULL*nid+nscid)%(1ULL<<31);
    return qpsk(gold(static_cast<std::uint32_t>(ci),2*count));
}
std::pair<double,double> pusch_power(int nprb,int mu,double p0,double alpha,
                                    double pl,double delta_tf,double f,double pcmax){
    require(nprb>=1&&nprb<=275&&mu>=0&&mu<=3&&alpha>=0&&alpha<=1&&
            std::isfinite(p0)&&std::isfinite(pl)&&std::isfinite(delta_tf)&&
            std::isfinite(f)&&std::isfinite(pcmax),"invalid power parameters");
    const double requested=p0+10*std::log10(std::pow(2.,mu)*nprb)+alpha*pl+delta_tf+f;
    return {requested,std::min(pcmax,requested)};
}

Symbols dft(const Symbols& input,bool inverse){
    require(!input.empty(),"empty DFT");
    const auto n=input.size();Symbols out(n);
    for(std::size_t k=0;k<n;++k){
        for(std::size_t j=0;j<n;++j)
            out[k]+=input[j]*std::polar(1.,(inverse?1.:-1.)*2*pi*double(k)*double(j)/double(n));
        out[k]/=std::sqrt(double(n));
    }
    return out;
}
Symbols fft(const Symbols& input,bool inverse){
    const auto n=input.size();require(n>0&&(n&(n-1))==0,"FFT requires power of two");
    Symbols out=input;
    for(std::size_t i=1,j=0;i<n;++i){
        std::size_t bit=n>>1;
        for(;j&bit;bit>>=1)j^=bit;
        j^=bit;if(i<j)std::swap(out[i],out[j]);
    }
    for(std::size_t len=2;len<=n;len*=2){
        const auto root=std::polar(1.,(inverse?1.:-1.)*2*pi/double(len));
        for(std::size_t i=0;i<n;i+=len){
            Complex w=1;
            for(std::size_t j=0;j<len/2;++j){
                const auto u=out[i+j],v=out[i+j+len/2]*w;
                out[i+j]=u+v;out[i+j+len/2]=u-v;w*=root;
            }
        }
    }
    for(auto& z:out)z/=std::sqrt(double(n));
    return out;
}
double papr_db(const Symbols& x){
    require(!x.empty(),"empty PAPR input");double peak=0,energy=0;
    for(auto z:x){energy+=std::norm(z);peak=std::max(peak,std::norm(z));}
    require(energy>0,"zero energy");return 10*std::log10(peak*double(x.size())/energy);
}
}
