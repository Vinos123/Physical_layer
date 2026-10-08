#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#include <immintrin.h>
#define LAB_AVX512 1
#endif
#if defined(__ARM_FEATURE_SVE)
#include <arm_sve.h>
#define LAB_SVE 1
#endif
using Kernel=void(*)(const float*,const float*,const float*,const float*,float*,float*,std::size_t);
// Six non-overlapping SoA buffers. Unaligned loads are supported.
void scalar(const float* ar,const float* ai,const float* br,const float* bi,float* cr,float* ci,std::size_t n) {
    for(std::size_t i=0;i<n;++i) {cr[i]=ar[i]*br[i]-ai[i]*bi[i];ci[i]=ar[i]*bi[i]+ai[i]*br[i];}
}
#ifdef LAB_AVX512
__attribute__((target("avx512f")))
void avx512(const float* ar,const float* ai,const float* br,const float* bi,float* cr,float* ci,std::size_t n) {
    std::size_t i=0;
    for(;i+16<=n;i+=16) {
        const auto a=_mm512_loadu_ps(ar+i),b=_mm512_loadu_ps(ai+i);
        const auto c=_mm512_loadu_ps(br+i),d=_mm512_loadu_ps(bi+i);
        _mm512_storeu_ps(cr+i,_mm512_sub_ps(_mm512_mul_ps(a,c),_mm512_mul_ps(b,d)));
        _mm512_storeu_ps(ci+i,_mm512_add_ps(_mm512_mul_ps(a,d),_mm512_mul_ps(b,c)));
    }
    scalar(ar+i,ai+i,br+i,bi+i,cr+i,ci+i,n-i);
}
#endif
#ifdef LAB_SVE
void sve(const float* ar,const float* ai,const float* br,const float* bi,float* cr,float* ci,std::size_t n) {
    for(std::size_t i=0;i<n;i+=svcntw()) {
        const auto pg=svwhilelt_b32(static_cast<std::uint64_t>(i),static_cast<std::uint64_t>(n));
        const auto a=svld1_f32(pg,ar+i),b=svld1_f32(pg,ai+i);
        const auto c=svld1_f32(pg,br+i),d=svld1_f32(pg,bi+i);
        svst1_f32(pg,cr+i,svsub_f32_x(pg,svmul_f32_x(pg,a,c),svmul_f32_x(pg,b,d)));
        svst1_f32(pg,ci+i,svadd_f32_x(pg,svmul_f32_x(pg,a,d),svmul_f32_x(pg,b,c)));
    }
}
#endif
int main() {
    constexpr std::size_t n=65539; // Deliberately not divisible by vector width.
    std::vector<float> ar(n),ai(n),br(n),bi(n),cr(n),ci(n),ref_r(n),ref_i(n);
    std::mt19937 rng(2026);std::uniform_real_distribution<float> random(-1,1);
    for(std::size_t i=0;i<n;++i) {ar[i]=random(rng);ai[i]=random(rng);br[i]=random(rng);bi[i]=random(rng);}
    scalar(ar.data(),ai.data(),br.data(),bi.data(),ref_r.data(),ref_i.data(),n);
    Kernel selected=scalar;const char* name="portable scalar source (compiler may auto-vectorize)";
#ifdef LAB_AVX512
    __builtin_cpu_init();
    if(__builtin_cpu_supports("avx512f")) {selected=avx512;name="AVX512F runtime dispatch";}
#endif
#ifdef LAB_SVE
    // This is an SVE-targeted binary. Deploy only on an SVE-capable CPU/OS.
    selected=sve;name="SVE compile-time target (variable vector length)";
#endif
    // Volatile dispatch prevents the compiler from eliding repeated identical calls.
    // The indirect-call cost is part of this simple educational benchmark.
    Kernel volatile dispatch=selected;
    auto run=[&]{dispatch(ar.data(),ai.data(),br.data(),bi.data(),cr.data(),ci.data(),n);};
    run();double maxerror=0;
    for(std::size_t i=0;i<n;++i) maxerror=std::max(maxerror,double(std::max(std::abs(cr[i]-ref_r[i]),std::abs(ci[i]-ref_i[i]))));
    if(maxerror>1e-6) throw std::runtime_error("kernel does not match scalar reference");
    for(int i=0;i<30;++i) run();
    std::vector<double> times;double checksum=0;
    constexpr int batches=101,repeats=100;
    for(int batch=0;batch<batches;++batch) {
        const auto begin=std::chrono::steady_clock::now();
        for(int r=0;r<repeats;++r) run();
        const auto end=std::chrono::steady_clock::now();
        checksum+=cr[static_cast<std::size_t>(batch)%n]+ci[static_cast<std::size_t>(batch)%n];
        times.push_back(std::chrono::duration<double,std::micro>(end-begin).count()/repeats);
    }
    std::sort(times.begin(),times.end());
    std::cout<<"kernel="<<name<<" n="<<n<<" max_abs_error="<<maxerror<<" checksum="<<checksum<<"\n"
             <<"batch_average_us p50="<<times[50]<<" p95="<<times[95]<<" p99="<<times[99]<<"\n"
             <<"These are percentiles of batch averages, not per-slot realtime latency.\n";
}
