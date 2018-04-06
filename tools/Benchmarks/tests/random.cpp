#include "stdafx.h"

#include <Random\MultiplyWithCarry.h>
#include <Random\XorShift.h>

void BM_rnd_rand(benchmark::State& state)
{
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(rand());
    }
}

void BM_rnd_multiplycarry(benchmark::State& state)
{
    core::random::MultiplyWithCarry mwc;

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(mwc.rand());
    }
}

void BM_rnd_xorshift(benchmark::State& state)
{
    core::random::XorShift mwc;

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(mwc.rand());
    }
}

void BM_rnd_xorshift128(benchmark::State& state)
{
    core::random::XorShift128 mwc;

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(mwc.rand());
    }
}

BENCHMARK(BM_rnd_rand);
BENCHMARK(BM_rnd_multiplycarry);
BENCHMARK(BM_rnd_xorshift);
BENCHMARK(BM_rnd_xorshift128);
