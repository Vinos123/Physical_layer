#include "pusch_lab.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace pusch {
namespace {
void require(bool b,const char* s) { if(!b) throw std::invalid_argument(s); }
void valid_bits(const Bits& b) {
    require(std::all_of(b.begin(),b.end(),[](Bit v){return v<=1;}), "bits must be 0/1");
}
struct CrcParams { unsigned width; std::uint32_t polynomial; };
CrcParams crc_params(Crc c) {
    switch(c) {
    case Crc::CRC16: return {16,0x1021};
    case Crc::CRC24A: return {24,0x864cfb};
    case Crc::CRC24B: return {24,0x800063};
    }
    throw std::invalid_argument("invalid CRC type");
}
}
LdpcPlan ldpc_plan(unsigned a,unsigned r) {
    require(a>0 && a<=2000000 && r>0 && r<=1024,"invalid LDPC planning input");
    LdpcPlan p{};
    p.a=a; p.tb_crc=a>3824 ? 24U : 16U; p.b=a+p.tb_crc;
    p.bg=(a<=292 || (a<=3824 && 100ULL*r<=67ULL*1024) || r<=256) ? 2U : 1U;
    const unsigned kcb=p.bg==1 ? 8448U : 3840U;
    p.c=p.b<=kcb ? 1U : (p.b+kcb-25)/(kcb-24);
    p.cb_crc=p.c==1 ? 0U : 24U;
    require(p.b%p.c==0,"B/C must be integral; input must be a legal compatible TBS");
    p.k_prime=p.b/p.c+p.cb_crc;
    p.kb=p.bg==1 ? 22U : p.b>640 ? 10U : p.b>560 ? 9U : p.b>192 ? 8U : 6U;
    constexpr std::array<unsigned,8> bases={2,3,5,7,9,11,13,15};
    p.zc=std::numeric_limits<unsigned>::max();
    for(unsigned i=0;i<bases.size();++i)
        for(unsigned z=bases[i];z<=384;z*=2)
            if(p.kb*z>=p.k_prime && z<p.zc) {p.zc=z;p.i_ls=i;}
    require(p.zc!=std::numeric_limits<unsigned>::max(),"no allowed lifting size");
    p.k=(p.bg==1 ? 22U : 10U)*p.zc;
    p.n=(p.bg==1 ? 66U : 50U)*p.zc;
    p.filler=p.k-p.k_prime;
    return p;
}
std::vector<unsigned> cb_output_lengths(std::uint64_t g,unsigned qm,unsigned nl,unsigned c) {
    require((qm==2 || qm==4 || qm==6 || qm==8) && nl>=1 && nl<=4 && c>0,
            "invalid CB output parameters");
    const auto unit=qm*nl;
    require(g>0 && g%unit==0,"G must be positive and divisible by Qm*layers");
    const auto symbols=g/unit, low=symbols/c, extra=symbols%c;
    std::vector<unsigned> e(c);
    for(unsigned j=0;j<c;++j) {
        const auto value=unit*(low+(j>=c-extra ? 1U : 0U));
        require(value<=std::numeric_limits<unsigned>::max(),"E overflow");
        e[j]=static_cast<unsigned>(value);
    }
    return e;
}
std::uint32_t crc_remainder(const Bits& b,Crc type) {
    valid_bits(b);
    const auto p=crc_params(type); const std::uint32_t mask=(1U<<p.width)-1;
    std::uint32_t reg=0;
    for(Bit bit:b) {
        const auto feedback=((reg>>(p.width-1))&1U)^bit;
        reg=(reg<<1U)&mask;
        if(feedback) reg^=p.polynomial;
    }
    return reg;
}
Bits attach_crc(const Bits& b,Crc type) {
    const auto p=crc_params(type); const auto reg=crc_remainder(b,type);
    Bits out=b;
    for(unsigned i=p.width;i>0;--i) out.push_back(static_cast<Bit>((reg>>(i-1))&1U));
    return out;
}
bool check_crc(const Bits& b,Crc type) {
    return b.size()>=crc_params(type).width && crc_remainder(b,type)==0;
}
Bits bytes_to_bits(const std::vector<std::uint8_t>& bytes) {
    Bits out; out.reserve(bytes.size()*8);
    for(auto byte:bytes)
        for(int j=7;j>=0;--j) out.push_back(static_cast<Bit>((byte>>j)&1));
    return out;
}
std::vector<Bits> segment_tb(const Bits& tb,const LdpcPlan& p) {
    valid_bits(tb); require(tb.size()==p.b,"TB+CRC length mismatch");
    require(p.c>0 && p.b%p.c==0 && p.k_prime<=p.k,"invalid segmentation plan");
    std::vector<Bits> out; const auto payload=p.b/p.c;
    for(unsigned r=0;r<p.c;++r) {
        Bits cb(tb.begin()+r*payload,tb.begin()+(r+1)*payload);
        if(p.cb_crc) cb=attach_crc(cb,Crc::CRC24B);
        require(cb.size()==p.k_prime,"CB+CRC length mismatch");
        cb.resize(p.k,0); // Filler is 0 for the encoder; NULL is a separate mask later.
        out.push_back(std::move(cb));
    }
    return out;
}
std::vector<std::uint8_t> filler_mask_after_puncture(const LdpcPlan& p) {
    std::vector<std::uint8_t> mask(p.n,0);
    for(unsigned k=p.k_prime;k<p.k;++k)
        if(k>=2*p.zc) mask[k-2*p.zc]=1;
    return mask;
}
RateMatched rate_match(const std::vector<std::int8_t>& d,unsigned bg,unsigned zc,
                       unsigned rv,unsigned e,unsigned qm) {
    require((bg==1 || bg==2) && zc>0 && zc<=384 && rv<=3,"invalid rate-match graph/RV");
    require((qm==2 || qm==4 || qm==6 || qm==8) && e>0 && e<=10000000 && e%qm==0,
            "invalid rate-match E/Qm");
    const unsigned n=(bg==1 ? 66U : 50U)*zc;
    require(d.size()==n,"input must contain N punctured LDPC positions");
    require(std::all_of(d.begin(),d.end(),[](auto v){return v>=-1 && v<=1;}),"invalid NULL/bit");
    require(std::any_of(d.begin(),d.end(),[](auto v){return v!=-1;}),"all positions are NULL");
    constexpr std::array<unsigned,4> bg1={0,17,33,56},bg2={0,13,25,43};
    unsigned j=(bg==1 ? bg1[rv] : bg2[rv])*zc;
    Bits selected; std::vector<unsigned> positions;
    selected.reserve(e); positions.reserve(e);
    while(selected.size()<e) {
        if(d[j]!=-1) { selected.push_back(static_cast<Bit>(d[j]));positions.push_back(j); }
        j=(j+1)%n;
    }
    RateMatched out; out.bits.resize(e);out.source_positions.resize(e);
    for(unsigned symbol=0;symbol<e/qm;++symbol)
        for(unsigned bit=0;bit<qm;++bit) {
            const auto to=symbol*qm+bit, from=bit*(e/qm)+symbol;
            out.bits[to]=selected[from];out.source_positions[to]=positions[from];
        }
    return out;
}
std::vector<double> rate_recover(const std::vector<double>& llr,
                                 const std::vector<unsigned>& pos,unsigned n) {
    require(n>0 && llr.size()==pos.size(),"rate-recovery size mismatch");
    std::vector<double> out(n,0);
    for(std::size_t i=0;i<llr.size();++i) {
        require(pos[i]<n,"rate-recovery position out of range");
        out[pos[i]]+=llr[i]; // Repetition and later HARQ observations add evidence.
    }
    return out;
}
} // namespace pusch
