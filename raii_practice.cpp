//
// Created by VINO on 2026/9/24.
//
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

struct Frame {
    explicit Frame(std::size_t count):samples(count, 1.0f) {
        std::cout << "Frame constructed" <<std::endl;
    }
    ~Frame(){std::cout << "Frame destructed" <<std::endl;}
    std::vector<float> samples;
};

int main() {
    auto owner = std::make_unique<Frame>(4);
    auto copy = owner->samples;
    //copy the data, copy have independent storage.
    copy[0] = 9.0f;
    std::cout<< "original first = " << owner->samples[0]<< std::endl;
    std::cout<< "copy first = " << copy[0]<< std::endl;

    auto next_owner = std::move(owner);
    //Transfer the ownership of the object managed by unique_pointer "Frame".
    std::cout<< std::boolalpha << "old owner empty = " << (owner == nullptr) << std::endl;
    std::cout << "new owner size = "<< next_owner->samples.size() << "\n";
    //Do not dereference owner after the move; Frame is destroyed automatically at scope exit, without delete.
    //std::move is a cast(static_cast); the type's constructor or assignment operator performs the actual move.
    return copy[0] == 9.0f && next_owner->samples[0] == 1.0f ? 0:1;
}