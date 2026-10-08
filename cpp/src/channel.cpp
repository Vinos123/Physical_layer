#include "pusch_lab.hpp"
#include <cmath>
#include <stdexcept>
namespace pusch {
namespace {
void check(const std::vector<Tap>& taps, double fs) {
    if(taps.empty() || !std::isfinite(fs) || fs<=0) throw std::invalid_argument("invalid channel geometry");
    double power=0;
    for(const auto& tap:taps) {
        if(!std::isfinite(tap.gain.real()) || !std::isfinite(tap.gain.imag()))
            throw std::invalid_argument("nonfinite tap");
        power+=std::norm(tap.gain);
    }
    if(power<=0) throw std::invalid_argument("zero-power channel");
}
}
std::vector<IQ> tapped_channel(const std::vector<IQ>& in,const std::vector<Tap>& taps,
                             double cfo,double fs,double variance,std::uint32_t seed) {
    check(taps,fs);
    if(!std::isfinite(cfo)) throw std::invalid_argument("nonfinite CFO");
    std::vector<IQ> out(in.size(),IQ{});
    for(std::size_t n=0;n<in.size();++n) {
        for(const auto& tap:taps) if(n>=tap.delay_samples) out[n]+=tap.gain*in[n-tap.delay_samples];
        out[n]*=std::polar(1.0,2*3.14159265358979323846*cfo*static_cast<double>(n)/fs);
    }
    return flat_awgn(out,IQ(1,0),variance,seed);
}
double rms_delay_seconds(const std::vector<Tap>& taps,double fs) {
    check(taps,fs);double p=0,mean=0;
    for(const auto& t:taps) {p+=std::norm(t.gain);mean+=std::norm(t.gain)*t.delay_samples/fs;}
    mean/=p;double variance=0;
    for(const auto& t:taps) {
        const auto d=t.delay_samples/fs-mean;variance+=std::norm(t.gain)*d*d;
    }
    return std::sqrt(variance/p);
}
} // namespace pusch
