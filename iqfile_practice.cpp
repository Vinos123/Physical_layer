//
// Created by VINO on 2026/9/25.
//
#include <iostream>
#include "include/phy.h"
#include <exception>

int main(int argc, char **argv) {
    try {
        //Without arguments, use output in the working directory
        std::filesystem::path path;
        if (argc>1) path = argv[1];
        else {
            std::filesystem::create_directories("output");
            path = "output/iq_samples.txt";
        }
        const std::vector<phy::IQ> samples{{3,4},{0.1f, -0.2f}, {1,0}};
        phy::write_iq(path, samples);
        //overwrite the specified file; use a dedicated practice path;
        const auto restored = phy::read_iq(path);
        std::cout << "samples = " <<restored.size() << std::endl;
        std::cout << " round trip equal = " << std::boolalpha << (samples == restored) << std::endl;
        return samples == restored ? 0 : 1;
    }catch (const std::exception &e) {
        std::cerr << "IQ file error: " << e.what() << std::endl;
        return 1;
    }
}
