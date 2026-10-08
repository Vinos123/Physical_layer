#include "pusch_lab.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace pusch {
namespace {
constexpr double pi = 3.14159265358979323846;
void require(bool ok, const char* message) {
    if (!ok) throw std::invalid_argument(message);
}
void bits_ok(const Bits& bits) {
    require(std::all_of(bits.begin(), bits.end(), [](Bit b){return b<=1;}), "bits must be 0/1");
}
void mapping_scope(const Config& c) {
    validate(c);
    require(c.layers==1 && (c.qm==2 || c.qm==4), "mapping lab supports one layer, QPSK/16QAM");
}
IQ constellation(const Bits& b, std::size_t offset, unsigned qm) {
    const double i_sign=1.0-2.0*b[offset], q_sign=1.0-2.0*b[offset+1];
    if(qm==2) return IQ(i_sign,q_sign)/std::sqrt(2.0);
    return IQ(i_sign*(2.0-(1.0-2.0*b[offset+2])),
              q_sign*(2.0-(1.0-2.0*b[offset+3])))/std::sqrt(10.0);
}
Bits label(unsigned v,unsigned qm) {
    Bits b(qm);
    for(unsigned i=0;i<qm;++i) b[i]=static_cast<Bit>((v>>(qm-1-i))&1U);
    return b;
}
std::size_t bin_for(unsigned k,unsigned width,unsigned nfft) {
    const auto signed_bin=static_cast<int>(k)-static_cast<int>(width/2);
    return static_cast<std::size_t>((signed_bin+static_cast<int>(nfft))%static_cast<int>(nfft));
}
}
Bits gold(std::uint32_t init,std::size_t length) {
    require(init<(1U<<31) && length<=10000000, "Gold input outside educational bounds");
    constexpr std::size_t nc=1600;
    Bits x1(nc+length+31,0),x2(x1.size(),0),out(length);
    x1[0]=1;
    for(unsigned i=0;i<31;++i) x2[i]=static_cast<Bit>((init>>i)&1U);
    for(std::size_t n=0;n<nc+length;++n) {
        x1[n+31]=x1[n+3]^x1[n];
        x2[n+31]=x2[n+3]^x2[n+2]^x2[n+1]^x2[n];
    }
    for(std::size_t n=0;n<length;++n) out[n]=x1[n+nc]^x2[n+nc];
    return out;
}
Bits scramble_data(const Bits& bits,std::uint16_t rnti,std::uint16_t nid) {
    bits_ok(bits);require(nid<=1023,"PUSCH data NID must be 0..1023");
    const auto sequence=gold((std::uint32_t(rnti)<<15)+nid,bits.size()); // q=0; no UCI placeholders.
    Bits out(bits.size());
    for(std::size_t i=0;i<bits.size();++i) out[i]=bits[i]^sequence[i];
    return out;
}
std::vector<IQ> qam_modulate(const Bits& bits,unsigned qm) {
    bits_ok(bits);require((qm==2 || qm==4) && bits.size()%qm==0,"invalid QAM input");
    std::vector<IQ> out(bits.size()/qm);
    for(std::size_t i=0;i<out.size();++i) out[i]=constellation(bits,i*qm,qm);
    return out;
}
Bits qam_hard_demodulate(const std::vector<IQ>& samples,unsigned qm) {
    require(qm==2 || qm==4,"hard demod supports QPSK/16QAM");
    Bits out;out.reserve(samples.size()*qm);
    for(IQ y:samples) {
        require(std::isfinite(y.real()) && std::isfinite(y.imag()),"nonfinite received IQ");
        double best=std::numeric_limits<double>::infinity();unsigned winner=0;
        for(unsigned v=0;v<(1U<<qm);++v) {
            const auto b=label(v,qm);const auto distance=std::norm(y-constellation(b,0,qm));
            if(distance<best) {best=distance;winner=v;}
        }
        const auto b=label(winner,qm);out.insert(out.end(),b.begin(),b.end());
    }
    return out;
}
std::vector<double> qam_maxlog_llr(const std::vector<IQ>& samples,unsigned qm,double variance) {
    require((qm==2 || qm==4) && std::isfinite(variance) && variance>0,"invalid LLR input");
    std::vector<double> out;out.reserve(samples.size()*qm);
    for(IQ y:samples) {
        require(std::isfinite(y.real()) && std::isfinite(y.imag()),"nonfinite received IQ");
        for(unsigned j=0;j<qm;++j) {
            double d0=std::numeric_limits<double>::infinity(),d1=d0;
            for(unsigned v=0;v<(1U<<qm);++v) {
                const auto b=label(v,qm);const auto d=std::norm(y-constellation(b,0,qm));
                (b[j]==0 ? d0 : d1)=std::min(b[j]==0 ? d0 : d1,d);
            }
            out.push_back((d1-d0)/variance); // log(P(bit=0)/P(bit=1)); positive means 0.
        }
    }
    return out;
}
std::vector<IQ> dmrs_type1(const Config& c) {
    mapping_scope(c);
    const std::uint64_t init=(1ULL<<17)*(14ULL*c.n_slot+c.dmrs_symbol+1)*
                            (2ULL*c.dmrs_nid+1)+2ULL*c.dmrs_nid;
    const auto offset=6ULL*(c.crb_start+c.start_prb);
    const auto count=6ULL*c.n_prbs;
    const auto sequence=gold(static_cast<std::uint32_t>(init&0x7fffffffULL),
                             static_cast<std::size_t>(2*(offset+count)));
    Bits selected(sequence.begin()+static_cast<std::ptrdiff_t>(2*offset),sequence.end());
    return qam_modulate(selected,2); // nSCID=0, beta=1, port 1000, ordinary type 1 (no enhancement).
}
std::vector<std::size_t> dmrs_indices(const Config& c) {
    mapping_scope(c);std::vector<std::size_t> out;
    const auto width=12*c.bwp_prbs;
    for(unsigned rb=c.start_prb;rb<c.start_prb+c.n_prbs;++rb)
        for(unsigned k=0;k<12;k+=2) out.push_back(std::size_t(c.dmrs_symbol)*width+12*rb+k);
    return out;
}
std::vector<std::size_t> data_indices(const Config& c) {
    mapping_scope(c);std::vector<std::size_t> out;const auto width=12*c.bwp_prbs;
    for(unsigned l=c.start_symbol;l<c.start_symbol+c.n_symbols;++l)
        for(unsigned rb=c.start_prb;rb<c.start_prb+c.n_prbs;++rb)
            for(unsigned k=0;k<12;++k)
                if(!(l==c.dmrs_symbol && k%2==0)) out.push_back(std::size_t(l)*width+12*rb+k);
    return out;
}
Grid map_grid(const Config& c,const std::vector<IQ>& data) {
    const auto indices=data_indices(c),pilots=dmrs_indices(c);
    const auto reference=dmrs_type1(c);
    require(data.size()==indices.size(),"data symbol count must match grid allocation");
    Grid grid(std::size_t(14)*12*c.bwp_prbs,IQ{});
    for(std::size_t i=0;i<data.size();++i) grid[indices[i]]=data[i];
    for(std::size_t i=0;i<pilots.size();++i) grid[pilots[i]]=reference[i];
    return grid;
}
std::vector<IQ> extract_data(const Config& c,const Grid& grid) {
    require(grid.size()==std::size_t(14)*12*c.bwp_prbs,"grid size mismatch");
    std::vector<IQ> out;for(auto index:data_indices(c)) out.push_back(grid[index]);return out;
}
IQ estimate_flat_channel(const Config& c,const Grid& grid) {
    require(grid.size()==std::size_t(14)*12*c.bwp_prbs,"grid size mismatch");
    const auto indices=dmrs_indices(c);const auto reference=dmrs_type1(c);
    IQ numerator{};double denominator=0;
    for(std::size_t i=0;i<indices.size();++i) {
        numerator+=grid[indices[i]]*std::conj(reference[i]);
        denominator+=std::norm(reference[i]);
    }
    return numerator/denominator; // Valid for constant flat channel, not frequency-selective fading.
}
void fft_inplace(std::vector<IQ>& values,bool inverse) {
    const auto n=values.size();require(n>0 && (n&(n-1))==0 && n<=65536,"FFT needs bounded power-of-two size");
    for(std::size_t i=1,j=0;i<n;++i) {
        auto bit=n>>1;
        for(;j&bit;bit>>=1) j^=bit;
        j^=bit;if(i<j) std::swap(values[i],values[j]);
    }
    for(std::size_t length=2;length<=n;length*=2) {
        const IQ step=std::polar(1.0,(inverse ? 2.0 : -2.0)*pi/static_cast<double>(length));
        for(std::size_t start=0;start<n;start+=length) {
            IQ w(1,0);
            for(std::size_t j=0;j<length/2;++j) {
                const IQ u=values[start+j],v=values[start+j+length/2]*w;
                values[start+j]=u+v;values[start+j+length/2]=u-v;w*=step;
            }
        }
    }
    // Unitary normalization in BOTH directions keeps noise variance/energy easy to trace.
    const auto scale=1/std::sqrt(static_cast<double>(n));for(auto& v:values) v*=scale;
}
std::vector<IQ> ofdm_modulate(const Grid& grid,unsigned prbs,unsigned nfft,unsigned cp) {
    require(prbs>=1 && prbs<=275 && 12*prbs<=nfft && cp<nfft,"invalid OFDM geometry");
    const auto width=12*prbs;require(grid.size()==std::size_t(14)*width,"grid size mismatch");
    std::vector<IQ> output;output.reserve(std::size_t(14)*(nfft+cp));
    for(unsigned l=0;l<14;++l) {
        std::vector<IQ> bins(nfft,IQ{});
        for(unsigned k=0;k<width;++k) bins[bin_for(k,width,nfft)]=grid[std::size_t(l)*width+k];
        fft_inplace(bins,true);
        output.insert(output.end(),bins.end()-cp,bins.end());
        output.insert(output.end(),bins.begin(),bins.end());
    }
    return output;
}
Grid ofdm_demodulate(const std::vector<IQ>& input,unsigned prbs,unsigned nfft,unsigned cp) {
    require(prbs>=1 && prbs<=275 && 12*prbs<=nfft && cp<nfft,"invalid OFDM geometry");
    require(input.size()==std::size_t(14)*(nfft+cp),"OFDM sample count mismatch");
    const auto width=12*prbs;Grid grid(std::size_t(14)*width);
    for(unsigned l=0;l<14;++l) {
        const auto offset=std::size_t(l)*(nfft+cp)+cp;
        std::vector<IQ> bins(input.begin()+static_cast<std::ptrdiff_t>(offset),
                             input.begin()+static_cast<std::ptrdiff_t>(offset+nfft));
        fft_inplace(bins,false);
        for(unsigned k=0;k<width;++k) grid[std::size_t(l)*width+k]=bins[bin_for(k,width,nfft)];
    }
    return grid;
}
std::vector<IQ> flat_awgn(const std::vector<IQ>& input,IQ h,double variance,std::uint32_t seed) {
    require(std::isfinite(variance) && variance>=0 && std::isfinite(h.real()) && std::isfinite(h.imag()),"invalid AWGN parameters");
    std::mt19937 rng(seed);std::normal_distribution<double> normal(0,1);
    const auto sigma=std::sqrt(variance/2);std::vector<IQ> output(input.size());
    for(std::size_t i=0;i<input.size();++i) {
        const double real=normal(rng),imag=normal(rng); // Explicit draw order, I then Q.
        output[i]=h*input[i]+IQ(sigma*real,sigma*imag);
    }
    return output;
}
double evm_rms(const std::vector<IQ>& reference,const std::vector<IQ>& received) {
    require(!reference.empty() && reference.size()==received.size(),"EVM input mismatch");
    double error=0,energy=0;
    for(std::size_t i=0;i<reference.size();++i) {error+=std::norm(received[i]-reference[i]);energy+=std::norm(reference[i]);}
    require(energy>0,"EVM reference has zero energy");return std::sqrt(error/energy);
}
double linear_mean_dbm(const std::vector<double>& values) {
    require(!values.empty(),"empty power measurements");
    for(double v:values) require(std::isfinite(v),"nonfinite power measurement");
    const auto peak=*std::max_element(values.begin(),values.end());double sum=0;
    for(double v:values) sum+=std::pow(10.0,(v-peak)/10);
    return peak+10*std::log10(sum/static_cast<double>(values.size()));
}
double pusch_power_dbm(double pmax,double p0,unsigned mu,unsigned prbs,double alpha,double pl,double delta,double f) {
    require(mu<=3 && prbs>=1 && prbs<=275 && alpha>=0 && alpha<=1,"invalid power-control input");
    for(double v:{pmax,p0,alpha,pl,delta,f}) require(std::isfinite(v),"nonfinite power-control input");
    return std::min(pmax,p0+10*std::log10(std::ldexp(static_cast<double>(prbs),static_cast<int>(mu)))+alpha*pl+delta+f);
}
} // namespace pusch
