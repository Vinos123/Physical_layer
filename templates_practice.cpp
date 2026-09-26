//
// Created by VINO on 2026/9/24.
//
#include<iostream>
#include<stdexcept>
#include<type_traits>
#include<vector>

template<class T>
std::vector<T> add(const std::vector<T>& a, const std::vector<T>& b) {
    static_assert(std::is_floating_point<T>::value, "Use float or double");
    if (a.size() != b.size()) throw std::invalid_argument("length mismatch");
    std::vector<T> result(a.size());
    for (std::size_t k = 0;k< a.size();++k)
        result[k] = a[k]+b[k];
    return result;
    //Allow return value optimization; an explicit std::move(result) is unnecessary.

}

template<class T>
std::vector<T> multiply(const std::vector<T>& a, T b) {
    static_assert(std::is_floating_point<T>::value, "Use float or double");
    std::vector<T> result(a.size());
    for (std::size_t k = 0;k< a.size();++k)
        result[k] = a[k]*b;
    return result;
}

int main() {
    auto f = add<float>({1,2}, {3,4}  );//test float
    auto d = add<double>({0.25,0.3}, {0.6,0.7}  );
    std::cout<<" add float result = {"<<f[0]<<", "<<f[1]<<"}\n";
    std::cout<<"add double result = {"<<d[0] <<", "<<d[1]<<"}\n";

    try {
        (void)add<float>({1},{2,3});
        return 1;
    }catch (const std::invalid_argument& e) {
        std::cout<<"caught: "<<e.what()<<"\n";
    }


    auto f2 = multiply<double>({3,1},0.6);
    std::cout<< "a * b = {"<<f2[0]<<", "<<f2[1]<<"}\n";

    return f[0] == 4 && f[1] == 6 && d[0] == 0.85 && d[1] ==1 && f2[0]==1.8 && f2[1] == 0.6 ? 0:1;
}