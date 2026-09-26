//
// Created by VINO on 2026/9/25.
//

#include "../include/phy.h"
#include <cmath>
#include <fstream>
#include <vector>

namespace phy {
    //calculate the energy of IQ signals.
    double energy(const std::vector<IQ>& samples) {
        double sum = 0.0;
        for (const auto& sample:samples) {
            sum += norm(sample);
        }
        return sum;
    }
    //calculate the output of fir filters.
    std::vector<float> fir(const std::vector<float>& input, const std::vector<float>& taps) {
        if (taps.empty()) throw std::invalid_argument("FIR taps cannot be empty");
        std::vector<float> output(input.size(),0.0);
        //initial output by the same size as input and fill it in zero.
        for (std::size_t n = 0 ;n < input.size(); ++n) {
            double sum = 0.0;
            for (std::size_t k=0;k < taps.size() && k<=n ; ++k) {
                sum += static_cast<double>(input[n-k]) * taps[k];
            }
            output[n] =static_cast<float>(sum);//y[n] = sum(h[k] * x[n-k])
        }
        return output;
    }
    //Write IQ samples into a file.
    void write_iq(const std::filesystem::path& path, const std::vector<IQ>& samples) {
        for (const auto& sample : samples) {
            if (!std::isfinite(sample.real())||!std::isfinite(sample.imag()))
                throw std::invalid_argument("IQ can not be infinite.");
        }
        std::ofstream out(path);
        out.imbue(std::locale::classic());
        if (!out) throw std::runtime_error("Can not open IQ output: " + path.string());
        out << std::setprecision(std::numeric_limits<float>::max_digits10);
        for (const auto& sample : samples) {
            out << sample.real() <<' ' << sample.imag() << '\n';
        }
        out.close();
        //Explicitly check final write errors; destruction still handles resource cleanup;
        if (!out) throw std::runtime_error("Can not write IQ output.");
    }
    //Read IQ samples from a file
    std::vector<IQ> read_iq(const std::filesystem::path& path) {
        std::ifstream in(path);
        if (!in) throw std::runtime_error(" Cannot open IQ input: " + path.string());
        std::vector<IQ> result;
        std::string line;
        std::size_t number =0;
        while (std::getline(in, line)) {
            ++number;
            std::istringstream row(line);
            row.imbue(std::locale::classic());
            row >> std::ws;//skip leading whitespace.
            if (row.eof()) continue;
            float i =0, q=0;
            if (!(row>>i>>q)||!std::isfinite(i)||!std::isfinite(q))
                throw std::runtime_error("invalid IQ pair at line " + std::to_string(number));
            row >> std::ws;//skip tail whitespace
            if (!row.eof())
                throw std::runtime_error("extra data at line " + std::to_string(number));
            result.emplace_back(i,q);//or push_back({i,q}).
        }
        if (in.bad())throw std::runtime_error("cannot read IQ input.");
        return result;
}

}
