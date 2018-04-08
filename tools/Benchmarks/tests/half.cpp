#include "stdafx.h"

#include <Math\XHalf.h>

void BM_half_from_float(benchmark::State& state)
{
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(core::XHalfCompressor::compress(74.25262f));
    }
}

void BM_half_to_float(benchmark::State& state)
{
    auto half = core::XHalfCompressor::compress(74.25262f);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(core::XHalfCompressor::decompress(half));
    }
}

void BM_half_from_float4(benchmark::State& state)
{
    Vec4f val(74.25262f, 0.25262f, -0.25262f, -252.25020f);

    while (state.KeepRunning())  {
        benchmark::DoNotOptimize(core::XHalfCompressor::compress(val));
    }
}

void BM_half_to_float4(benchmark::State& state)
{
    Vec4f val(74.25262f, 0.25262f, -0.25262f, -252.25020f);
    auto half = core::XHalfCompressor::compress(val);

    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(core::XHalfCompressor::decompress(half));
    }
}


BENCHMARK(BM_half_from_float);
BENCHMARK(BM_half_to_float);
BENCHMARK(BM_half_from_float4);
BENCHMARK(BM_half_to_float4);