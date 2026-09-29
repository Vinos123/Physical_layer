#pragma once
#include <complex>
#include <cstdint>
#include <utility>
#include <vector>

// Teaching profile: ordinary single-slot PUSCH, 1 codeword, <=4 layers.
// No LDPC codec, UCI mux, DCI parser or complete NR receiver is provided.
namespace nr {
using Complex = std::complex<double>;
using Bits = std::vector<std::uint8_t>;
using Symbols = std::vector<Complex>;
struct Budget {
    int re_tbs, re_actual, tbs, g; // re_actual/g = -1 when overhead is nonzero
    double ninfo;
};
struct LDPCSizes {
    int a,tb_crc,b,bg,c,cb_crc,kprime,kb,zc,k,filler,n;
};
int tbs_from_ninfo(double ninfo,double rate);
Budget resource_budget(int nprb=20,int symbols=12,int dmrs_re=12,
                       int overhead=0,int qm=4,double rate=490.0/1024,int layers=1);
LDPCSizes ldpc_sizes(int a,double rate);
int riv(int n,int start,int length);
int sliv(int start,int length);
Bits gold(std::uint32_t cinit,int count);
Symbols qpsk(const Bits& bits);
Bits hard_qpsk(const Symbols& symbols);
Symbols dmrs_cp(int slot,int symbol,int count,int nid=42,int nscid=0);
std::pair<double,double> pusch_power(int nprb,int mu,double p0,double alpha,
    double pl,double delta_tf=0,double f=0,double pcmax=23);
Symbols dft(const Symbols& input,bool inverse=false); // unitary, O(N^2) reference
Symbols fft(const Symbols& input,bool inverse=false); // unitary, radix-2
double papr_db(const Symbols& x);
}
