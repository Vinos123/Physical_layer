//
// Created by VINO on 2026/9/26.
//
#include "include/phy.h"
#include <iostream>

int main() {
    const std::vector<phy::IQ> samples{{3,4},{1,0}};
    //Set a breakpoint on the next line or inside phy::energy;watch sum change from 0 to 25, then 26.
    const auto result = phy::energy(samples);
    std::cout << "result = " <<result << std::endl;
    return result == 26? 0:1;
}