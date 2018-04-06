#include "stdafx.h"

void BM_str_strlen_std(benchmark::State& state)
{
    std::string s1(state.range(0), '-');

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(std::strlen(s1.c_str()));
    }
}

void BM_str_strlen_core(benchmark::State& state)
{
    std::string s1(state.range(0), '-');

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(core::strUtil::strlen(s1.c_str()));
    }
}

void BM_str_compare_std(benchmark::State& state)
{
    std::string s1(state.range(0), '-');
    std::string s2(state.range(0), '-');

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(s1.compare(s2));
    }
}

void BM_str_compare_core(benchmark::State& state)
{
    std::string s1(state.range(0), '-');
    std::string s2(state.range(0), '-');

    const char* p1Begin = s1.c_str();
    const char* p1End = s1.c_str() + s1.length();

    const char* p2Begin = s2.c_str();
    const char* p2End = s2.c_str() + s2.length();

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(core::strUtil::IsEqual(p1Begin, p1End, p2Begin, p2End));
    }
}

BENCHMARK(BM_str_strlen_std)->Range(1, 1 << 16);
BENCHMARK(BM_str_strlen_core)->Range(1, 1 << 16);

BENCHMARK(BM_str_compare_std)->Range(1, 1 << 16);
BENCHMARK(BM_str_compare_core)->Range(1, 1 << 16);
