#include "stdafx.h"


void BM_memcpy(benchmark::State& state) {
	
	char* pSrc = X_NEW_ARRAY_ALIGNED(char, state.range(0), g_arena, "TestSrc", 64);
	char* pDst = X_NEW_ARRAY_ALIGNED(char, state.range(0), g_arena, "TestSrc", 64);

	std::memset(pSrc, 'x', state.range(0));

	const size_t num = state.range(0);

	while (state.KeepRunning()) {
		std::memcpy(pDst, pSrc, num);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(state.range(0)));

	X_DELETE_ARRAY(pSrc, g_arena);
	X_DELETE_ARRAY(pDst, g_arena);
}

void BM_memcpy_SIMD(benchmark::State& state) {

	char* pSrc = X_NEW_ARRAY_ALIGNED(char, state.range(0), g_arena, "TestSrc", 64);
	char* pDst = X_NEW_ARRAY_ALIGNED(char, state.range(0), g_arena, "TestSrc", 64);

	std::memset(pSrc, 'x', state.range(0));

	const size_t num = state.range(0) >> 4;

	while (state.KeepRunning()) {
		core::Mem::SIMDMemCopy(pDst, pSrc, num);
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(state.range(0)));

	X_DELETE_ARRAY(pSrc, g_arena);
	X_DELETE_ARRAY(pDst, g_arena);
}

BENCHMARK(BM_memcpy)->RangeMultiplier(2)->Range(8 << 20, 8 << 26);
BENCHMARK(BM_memcpy_SIMD)->RangeMultiplier(2)->Range(8 << 20, 8 << 26);
