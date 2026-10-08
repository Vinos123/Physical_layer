#include "pusch_lab.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace pusch {
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}
unsigned floor_log2(std::uint64_t x) {
    require(x > 0, "log2 input must be positive");
    unsigned n = 0;
    while (x >>= 1U) ++n;
    return n;
}
std::uint64_t ceil_div(std::uint64_t a, std::uint64_t b) { return (a+b-1)/b; }
constexpr std::array<unsigned,93> small_tbs = {
    24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,
    192,208,224,240,256,272,288,304,320,336,352,368,384,408,432,456,480,504,528,
    552,576,608,640,672,704,736,768,808,848,888,928,984,1032,1064,1128,1160,1192,
    1224,1256,1288,1320,1352,1416,1480,1544,1608,1672,1736,1800,1864,1928,2024,
    2088,2152,2216,2280,2408,2472,2536,2600,2664,2728,2792,2856,2976,3104,3240,
    3368,3496,3624,3752,3824};
}
Mcs mcs_table1(unsigned index) {
    constexpr std::array<unsigned,29> rates = {
        120,157,193,251,308,379,449,526,602,679,340,378,434,490,553,616,658,
        438,466,517,567,616,666,719,772,822,873,910,948};
    require(index < rates.size(), "MCS table 1 first-transmission index must be 0..28");
    return {index < 10 ? 2U : index < 17 ? 4U : 6U, rates[index]};
}
unsigned riv_encode(unsigned n, unsigned s, unsigned l) {
    require(n>=1 && n<=275 && s<n && l>=1 && l<=n-s, "invalid RIV allocation");
    return l-1 <= n/2 ? n*(l-1)+s : n*(n-l+1)+(n-1-s);
}
std::pair<unsigned,unsigned> riv_decode(unsigned n, unsigned v) {
    require(n>=1 && n<=275, "invalid BWP size");
    // Brute-force reference implementation: unambiguous, easy to compare with a fast decoder.
    for (unsigned l=1; l<=n; ++l)
        for (unsigned s=0; s+l<=n; ++s)
            if (riv_encode(n,s,l)==v) return {s,l};
    throw std::invalid_argument("RIV has no valid allocation for this BWP");
}
unsigned sliv_encode(unsigned s, unsigned l) {
    require(s<14 && l>=1 && l<=14-s, "invalid SLIV");
    return l-1<=7 ? 14*(l-1)+s : 14*(15-l)+13-s;
}
std::pair<unsigned,unsigned> sliv_decode(unsigned v) {
    for(unsigned l=1;l<=14;++l)
        for(unsigned s=0;s+l<=14;++s)
            if(sliv_encode(s,l)==v) return {s,l};
    throw std::invalid_argument("SLIV has no valid 14-symbol allocation");
}
void validate(const Config& c) {
    require(c.bwp_prbs>=1 && c.bwp_prbs<=275, "BWP size must be 1..275");
    require(c.n_prbs>=1 && c.start_prb<c.bwp_prbs && c.n_prbs<=c.bwp_prbs-c.start_prb,
            "PRB allocation exceeds BWP");
    require(c.n_symbols>=1 && c.start_symbol<14 && c.n_symbols<=14-c.start_symbol,
            "symbol allocation exceeds slot");
    require(c.dmrs_symbol>=c.start_symbol && c.dmrs_symbol<c.start_symbol+c.n_symbols,
            "DM-RS must be inside allocated symbols");
    require(c.qm==2 || c.qm==4 || c.qm==6 || c.qm==8, "unsupported modulation order");
    require(c.layers>=1 && c.layers<=4 && c.mu<=3, "invalid educational layer/numerology scope");
    require(c.r_x1024>=1 && c.r_x1024<=1024, "invalid target rate");
    require(c.x_overhead==0 || c.x_overhead==6 || c.x_overhead==12 || c.x_overhead==18,
            "xOverhead must be 0/6/12/18");
    require(c.x_overhead<12*c.n_symbols-6, "nonpositive TBS resource budget");
    require(c.data_nid<=1023 && c.crb_start<=2199, "identifier/reference outside lab scope");
    require(c.n_slot<10*(1U<<c.mu), "n_slot is a slot index within a radio frame");
}
Budget resource_budget(const Config& c) {
    validate(c);
    const unsigned actual = 12*c.n_symbols-6;
    const auto data = std::uint64_t(actual)*c.n_prbs;
    const auto re = std::uint64_t(std::min(156U,actual-c.x_overhead))*c.n_prbs;
    return {actual,data,re,data*c.qm*c.layers};
}
unsigned tbs_from_re(std::uint64_t re, Mcs mcs, unsigned layers) {
    require(re>0 && re<=156U*275U && layers>=1 && layers<=4, "TBS resource scope exceeded");
    require((mcs.qm==2 || mcs.qm==4 || mcs.qm==6 || mcs.qm==8) &&
            mcs.r_x1024>=1 && mcs.r_x1024<=1024, "invalid MCS");
    // Keep R_x1024 exact; avoid floating-point errors at quantization boundaries.
    const std::uint64_t numerator = re*mcs.qm*layers*mcs.r_x1024;
    if(numerator<=3824ULL*1024) {
        const int exponent = static_cast<int>(floor_log2(numerator))-10-6;
        const auto step = 1ULL<<static_cast<unsigned>(std::max(3,exponent));
        const auto quantized = std::max(24ULL, step*(numerator/(1024*step)));
        return *std::lower_bound(small_tbs.begin(),small_tbs.end(),quantized);
    }
    const auto shifted = numerator-24ULL*1024;
    const unsigned n = floor_log2(shifted)-10-5;
    const auto step = 1ULL<<n, denominator = step*1024;
    // Round positive values to nearest; exact ties round up.
    const auto quantized = std::max(3840ULL,step*((shifted+denominator/2)/denominator));
    std::uint64_t c=1;
    if(mcs.r_x1024<=256) c=ceil_div(quantized+24,3816);
    else if(quantized>8424) c=ceil_div(quantized+24,8424);
    const auto result=8*c*ceil_div(quantized+24,8*c)-24;
    require(result<=std::numeric_limits<unsigned>::max(), "TBS overflow");
    return static_cast<unsigned>(result);
}
} // namespace pusch

