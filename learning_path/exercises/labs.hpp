#pragma once
#include <complex>
#include <cstdint>
#include <vector>
namespace learner {
using Bits = std::vector<std::uint8_t>;
using IQ = std::complex<double>;
// L01: Data RE in [0,12) symbols, 10 PRBs at BWP PRB 5; even k of l=2 reserved.
std::vector<unsigned> data_re();
// L02: 38.211 Gold sequence, Nc=1600, c_init < 2^31.
Bits gold(std::uint32_t init, unsigned count);
// L03: 38.211 16QAM; one symbol from four unpacked bits.
IQ qam16(const Bits& bits);
// L04: Port 1000 type-1 CP-OFDM DMRS c_init; nSCID=0, normal CP.
std::uint32_t dmrs_init(unsigned slot, unsigned symbol, unsigned nid);
// L05: Flat-channel LS; input pilots have unit energy, equal nonzero length.
IQ flat_ls(const std::vector<IQ>& x, const std::vector<IQ>& y);
// L06: CRC16 0x1021, initial zero, MSB first, no reflection/final xor.
std::uint16_t crc16(const Bits& bits);
// L07: A check node of normalized min-sum (not a complete NR LDPC decoder).
std::vector<double> check_node(const std::vector<double>& q, double alpha);
// L08: BG2, Zc=2, Ncb=N=100, no filler, QPSK, RV 0..3.
// Return mother-code positions AFTER bit interleaving, E even.
std::vector<unsigned> rm_positions(unsigned rv, unsigned e);
// L09: Accumulate repeated observations to mother-code coordinates.
std::vector<double> combine(const std::vector<unsigned>& pos,
                            const std::vector<double>& llr, unsigned n);
// L10: QPSK max-log/exact LLR with positive=>bit0, variance E[|n|^2].
std::vector<double> qpsk_llr(IQ y, double noise_variance);
}
