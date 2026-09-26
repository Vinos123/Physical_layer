//
// Created by VINO on 2026/9/25.
//

#ifndef INC_4G_5G_PHYSICAL_LAYER_PHY_H
#define INC_4G_5G_PHYSICAL_LAYER_PHY_H
#include <complex>
#include <filesystem>

namespace phy {
    using IQ = std::complex<float>;
    double energy(const std::vector<IQ>& samples);
    //Casual FIR: input before the block is zero; output length equals input length, excluding the convolution tail.
    //Calls are independent with no inter-block state; empty taps are invalid, while empty input gives empty output.
    std::vector<float> fir(const std::vector<float>& input, const std::vector<float>& taps);
    void write_iq(const std::filesystem::path& path, const std::vector<IQ>& samples);
    std::vector<IQ> read_iq(const std::filesystem::path& path);
}



#endif //INC_4G_5G_PHYSICAL_LAYER_PHY_H
