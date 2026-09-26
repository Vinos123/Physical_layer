//
// Created by VINO on 2026/9/24.
//
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>

std::int16_t to_q15(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument{"non-finite input"};
    }
    const double clipped = std::clamp(value, -1.0, 32767.0 / 32768.0);//限制范围，避免溢出
    return static_cast<std::int16_t>(std::lround(clipped * 32768.0));
}
//Q15 in this example: real value = signed 16-bit integer /32768
//Range: [-1, 32767/32768].Clamp before conversion to avoid conversion overflow.

double q15_to_double(std::int16_t value) noexcept {
    return static_cast<double>(value) / 32768.0;
}

std::int16_t multiply_q15(std::int16_t a, std::int16_t b) {
    const std::int32_t product = static_cast<std::int32_t>(a) *b;
    // Use division to avoid implementation-defined right shifts of negative signed integers in C++17.
    //This example truncates toward zero; prodcution code may require a different rounding policy.
    const std::int32_t scaled = product / 32768.0;
    return static_cast<std::int16_t>(std::clamp(scaled, std::int32_t{-32768},std::int32_t{32767}));
}


int main() {
    auto half = to_q15(0.5);
    std::cout << " Q15(0.5) =  "<< half << std::endl;
    auto quarter = multiply_q15(half, half);
    std::cout << " Q15(0.25) = "<< quarter << std::endl;
    auto saturated = multiply_q15(to_q15(-1), to_q15(-1));
    std::cout << "saturated (-1) * (-1) raw = " << saturated << std::endl;


    auto test_01 = to_q15(0.1);
    std::cout << " test_01  = " << test_01 << std::endl;
    std::cout << "q15_to_double = " << q15_to_double(test_01) << std::endl;
    std::cout <<" q15(1.2) = " <<to_q15(1.2) << std::endl;
    std::cout << "q15(-1.2) = " << to_q15(-1.2) << std::endl;

    double energy = 0.0;
    std::int64_t accumulator = 0;
    for (size_t k = 0;k<100;++k) {
        double x = std::sin(2 * std::numbers::pi * static_cast<double>(k)  / 100.0);
        energy += x * x;
        std::int16_t x_q15 = to_q15(x);
        accumulator += static_cast<std::int64_t>(x_q15) * x_q15 ;

    }
   double fixed_energy = static_cast<double>(accumulator) / (32768.0 * 32768.0);
    //Every Q15 Sample is scaled by 32768, so the sum of the squares should be divided by 32768^2.
    double error = std::abs(fixed_energy - energy);
    if (error != 0.0) {
        std::cout<< "relative error = " <<error << std::endl;
    }
    std::cout << "fixed_energy = " << fixed_energy << std::endl;
    std::cout <<" energy = " << energy << std::endl;
    return half == 16384 && quarter == 8192 && saturated == 32767 ? 0:1;
}
