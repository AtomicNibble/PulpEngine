#include "stdafx.h"

#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>

core::GrowingBlockAllocator allocator;

void BM_mem_a_malloc(benchmark::State& state) {

	const size_t size = state.range(0);

	while (state.KeepRunning()) {
		void* pMem = malloc(size); 
		free(pMem);
	}
}

void BM_mem_a_arena(benchmark::State& state) {

	const size_t size = state.range(0);

	auto* pArena = g_arena;

	while (state.KeepRunning()) {
		char* pMem = X_NEW_ARRAY(char, size, pArena, "test");
		X_NAMESPACE(core)::Mem::DeleteArray(pMem, pArena);
	}
}

void BM_mem_a_allocator(benchmark::State& state) {

	const size_t size = state.range(0);

	auto* pArena = g_arena;

	while (state.KeepRunning()) {
		void* pMem = allocator.allocate(size, X_ALIGN_OF(char), 0);
		allocator.free(pMem);
	}
}


// BENCHMARK(BM_mem_a_malloc)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_mem_a_arena)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_mem_a_allocator)->RangeMultiplier(2)->Range(16, 1 << 16);
