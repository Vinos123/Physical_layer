// Original tutorial solutions. Existing pusch_lab functions serve as reference adapters.
#include "labs.hpp"
#include "pusch_lab.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace learner {
std::vector<unsigned> data_re() {
    const auto idx=pusch::data_indices(pusch::Config{});
    return {idx.begin(),idx.end()};
}
Bits gold(std::uint32_t init,unsigned count) { return pusch::gold(init,count); }
IQ qam16(const Bits& b) {
    if(b.size()!=4) throw std::invalid_argument("four bits required");
    return pusch::qam_modulate(b,4).front();
}
std::uint32_t dmrs_init(unsigned slot,unsigned symbol,unsigned nid) {
    if(symbol>=14 || nid>65535) throw std::invalid_argument("DMRS arguments");
    const std::uint64_t v=(14ULL*slot+symbol+1)*(2ULL*nid+1)*(1ULL<<17)+2ULL*nid;
    return static_cast<std::uint32_t>(v&0x7fffffffULL);
}
IQ flat_ls(const std::vector<IQ>& x,const std::vector<IQ>& y) {
    if(x.empty() || x.size()!=y.size()) throw std::invalid_argument("pilot sizes");
    IQ sum{};double energy=0;
    for(unsigned i=0;i<x.size();++i) { sum+=std::conj(x[i])*y[i];energy+=std::norm(x[i]); }
    if(!(energy>0)) throw std::invalid_argument("zero pilot energy");
    return sum/energy;
}
std::uint16_t crc16(const Bits& b) { return static_cast<std::uint16_t>(pusch::crc_remainder(b,pusch::Crc::CRC16)); }
std::vector<double> check_node(const std::vector<double>& q,double alpha) {
    if(q.size()<2 || !(alpha>0 && alpha<=1)) throw std::invalid_argument("check node arguments");
    for(double v:q) if(!std::isfinite(v)) throw std::invalid_argument("finite messages required");
    std::vector<double> r(q.size());
    for(unsigned j=0;j<q.size();++j) {
        double mag=std::numeric_limits<double>::infinity();bool negative=false;
        for(unsigned k=0;k<q.size();++k) if(k!=j) {mag=std::min(mag,std::abs(q[k]));negative^=q[k]<0;}
        r[j]=(negative?-1:1)*alpha*mag;
    }
    return r;
}
std::vector<unsigned> rm_positions(unsigned rv,unsigned e) {
    if(rv>3 || e==0 || e%2) throw std::invalid_argument("RV or E");
    return pusch::rate_match(std::vector<std::int8_t>(100,0),2,2,rv,e,2).source_positions;
}
std::vector<double> combine(const std::vector<unsigned>& p,const std::vector<double>& l,unsigned n) {
    return pusch::rate_recover(l,p,n);
}
std::vector<double> qpsk_llr(IQ y,double v) {
    if(!(v>0) || !std::isfinite(v)) throw std::invalid_argument("noise variance");
    return {2*std::sqrt(2.0)*y.real()/v,2*std::sqrt(2.0)*y.imag()/v};
}
}
