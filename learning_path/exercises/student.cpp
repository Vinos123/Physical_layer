#include "labs.hpp"
#include <stdexcept>
namespace learner {
std::vector<unsigned> data_re() { throw std::logic_error("TODO L01: chapter 20"); }
Bits gold(std::uint32_t, unsigned) { throw std::logic_error("TODO L02: chapter 21"); }
IQ qam16(const Bits&) { throw std::logic_error("TODO L03: chapter 21"); }
std::uint32_t dmrs_init(unsigned,unsigned,unsigned) { throw std::logic_error("TODO L04: chapter 22"); }
IQ flat_ls(const std::vector<IQ>&,const std::vector<IQ>&) { throw std::logic_error("TODO L05: chapter 23"); }
std::uint16_t crc16(const Bits&) { throw std::logic_error("TODO L06: chapter 24"); }
std::vector<double> check_node(const std::vector<double>&,double) { throw std::logic_error("TODO L07: chapter 25"); }
std::vector<unsigned> rm_positions(unsigned,unsigned) { throw std::logic_error("TODO L08: chapter 26"); }
std::vector<double> combine(const std::vector<unsigned>&,const std::vector<double>&,unsigned) { throw std::logic_error("TODO L09: chapter 26"); }
std::vector<double> qpsk_llr(IQ,double) { throw std::logic_error("TODO L10: chapter 27"); }
}
