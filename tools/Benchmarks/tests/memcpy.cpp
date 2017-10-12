#include "stdafx.h"


void BM_memcpy(benchmark::State& state) {

	char* pSrc = new char[state.range(0)];
	char* pDst = new char[state.range(0)];

	std::memset(pSrc, 'x', state.range(0));

	while (state.KeepRunning()) {
		std::memcpy(pDst, pSrc, state.range(0));
	}

	state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) + static_cast<int64_t>(state.range(0)));

	delete[] pSrc;
	delete[] pDst;
}

BENCHMARK(BM_memcpy)->RangeMultiplier(2)->Range(8, 8 << 10);