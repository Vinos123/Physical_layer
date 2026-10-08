#pragma once
#include <complex>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// Original educational implementation. Scope and normative clauses: cpp/README.md.
namespace pusch {
using Bit = std::uint8_t;        // Unpacked bit, strictly 0 or 1.
using Bits = std::vector<Bit>;
using IQ = std::complex<double>;

struct Mcs { unsigned qm; unsigned r_x1024; };
Mcs mcs_table1(unsigned index);  // First transmission, indices 0..28 only.
unsigned riv_encode(unsigned n_bwp, unsigned start, unsigned length);
std::pair<unsigned,unsigned> riv_decode(unsigned n_bwp, unsigned riv);
unsigned sliv_encode(unsigned start, unsigned length); // 14-symbol normal-CP grid.
std::pair<unsigned,unsigned> sliv_decode(unsigned sliv);

struct Config {
    unsigned bwp_prbs = 50, start_prb = 5, n_prbs = 10;
    unsigned start_symbol = 0, n_symbols = 12, dmrs_symbol = 2;
    unsigned qm = 2, layers = 1, r_x1024 = 526, x_overhead = 0;
    unsigned mu = 1, n_slot = 2;
    unsigned crb_start = 0; // BWP offset from CRB 0 for CP-OFDM DM-RS sequence.
    std::uint16_t rnti = 0x1234, data_nid = 42, dmrs_nid = 42;
};
void validate(const Config& cfg);
struct Budget {
    unsigned actual_re_per_prb;
    std::uint64_t actual_data_re, tbs_re, g;
};
Budget resource_budget(const Config& cfg); // One type-1 DM-RS symbol, one CDM group.
unsigned tbs_from_re(std::uint64_t re, Mcs mcs, unsigned layers = 1);

struct LdpcPlan {
    unsigned a, tb_crc, b, bg, c, cb_crc, k_prime, kb, zc, i_ls, k, n, filler;
};
LdpcPlan ldpc_plan(unsigned a, unsigned r_x1024);
std::vector<unsigned> cb_output_lengths(std::uint64_t g, unsigned qm,
                                        unsigned layers, unsigned c);
enum class Crc { CRC16, CRC24A, CRC24B };
std::uint32_t crc_remainder(const Bits& bits, Crc type);
Bits attach_crc(const Bits& bits, Crc type);
bool check_crc(const Bits& bits_with_crc, Crc type);
Bits bytes_to_bits(const std::vector<std::uint8_t>& bytes); // MSB first.
std::vector<Bits> segment_tb(const Bits& tb_with_crc, const LdpcPlan& plan);
std::vector<std::uint8_t> filler_mask_after_puncture(const LdpcPlan& plan);

// Integration contract; no fake LDPC encoder is provided.
class LdpcEncoderBackend {
public:
    virtual ~LdpcEncoderBackend() = default;
    // K unpacked bits (filler set to 0) -> N bits, first 2*Zc already punctured.
    virtual Bits encode(const Bits& cb, const LdpcPlan& plan) = 0;
};
struct RateMatched { Bits bits; std::vector<unsigned> source_positions; };
// d has N=66Zc/50Zc positions; -1 is NULL. Supports Ncb=N, no CBG/LBRM.
RateMatched rate_match(const std::vector<std::int8_t>& d, unsigned bg, unsigned zc,
                       unsigned rv, unsigned e, unsigned qm);
std::vector<double> rate_recover(const std::vector<double>& llr,
                                 const std::vector<unsigned>& positions, unsigned n);

Bits gold(std::uint32_t c_init, std::size_t length); // 38.211 §5.2.1, Nc=1600.
Bits scramble_data(const Bits& bits, std::uint16_t rnti, std::uint16_t nid);
std::vector<IQ> qam_modulate(const Bits& bits, unsigned qm); // QPSK / 16QAM.
Bits qam_hard_demodulate(const std::vector<IQ>& values, unsigned qm);
std::vector<double> qam_maxlog_llr(const std::vector<IQ>& values, unsigned qm,
                                  double complex_noise_variance);
std::vector<IQ> dmrs_type1(const Config& cfg); // CP-OFDM, port 1000, nSCID=0.
std::vector<std::size_t> data_indices(const Config& cfg);
std::vector<std::size_t> dmrs_indices(const Config& cfg);
using Grid = std::vector<IQ>; // grid[l * (12*bwp_prbs) + k].
Grid map_grid(const Config& cfg, const std::vector<IQ>& data);
std::vector<IQ> extract_data(const Config& cfg, const Grid& grid);
IQ estimate_flat_channel(const Config& cfg, const Grid& received);
void fft_inplace(std::vector<IQ>& values, bool inverse);
// Educational uniform CP; not the complete NR sample timing / phase compensation.
std::vector<IQ> ofdm_modulate(const Grid& grid, unsigned bwp_prbs,
                              unsigned nfft = 1024, unsigned cp = 72);
Grid ofdm_demodulate(const std::vector<IQ>& samples, unsigned bwp_prbs,
                    unsigned nfft = 1024, unsigned cp = 72);
std::vector<IQ> flat_awgn(const std::vector<IQ>& samples, IQ h,
                         double time_noise_variance, std::uint32_t seed);
double evm_rms(const std::vector<IQ>& reference, const std::vector<IQ>& received);
double linear_mean_dbm(const std::vector<double>& values_dbm);
double pusch_power_dbm(double pmax, double p0, unsigned mu, unsigned prbs,
                       double alpha, double pl, double delta_tf, double f);
struct Tap { unsigned delay_samples; IQ gain; };
// Causal, same-length output; missing history is zero, convolution tail is dropped.
// Static integer-delay SISO FIR + CFO + AWGN. Not a 38.901 TDL implementation.
std::vector<IQ> tapped_channel(const std::vector<IQ>& input,
                              const std::vector<Tap>& taps, double cfo_hz,
                              double sample_rate_hz, double noise_variance,
                              std::uint32_t seed);
double rms_delay_seconds(const std::vector<Tap>& taps, double sample_rate_hz);
} // namespace pusch
