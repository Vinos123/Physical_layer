//
// Created by VINO on 2026/9/25.
//
#include "include/phy.h"
#include <iostream>

int main() {
    const std::vector<float> impulse{1,0,0,0,0};
    const std::vector<float> taps{0.25f,0.5f,0.25f};
    const auto output = phy::fir(impulse, taps);
    std::cout << "Impulse response:";
    for (float value:output) std::cout << ' ' << value;
    std::cout << std::endl;
    return output == std::vector<float>{0.25f,0.5f,0.25f, 0,0}?0:1;
}