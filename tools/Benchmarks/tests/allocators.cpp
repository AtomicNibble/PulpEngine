#include "stdafx.h"

#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\SimpleMemoryArena.h>


void BM_mem_malloc(benchmark::State& state) {

	const size_t size = state.range(0);

	while (state.KeepRunning()) {
		void* pMem = malloc(size); 
		free(pMem);
	}
}


void BM_mem_blockallocator(benchmark::State& state) {

	const size_t size = state.range(0);

	core::GrowingBlockAllocator allocator;

	while (state.KeepRunning()) {
		void* pMem = allocator.allocate(size, X_ALIGN_OF(char), 0);
		allocator.free(pMem);
	}
}

void BM_mem_blockallocator_simplearena(benchmark::State& state) {

	const size_t size = state.range(0);

	typedef core::SimpleMemoryArena<core::GrowingBlockAllocator> BlockArena;

	BlockArena::AllocationPolicy allocator;
	BlockArena arena(&allocator, "BenchmarkArena");

	while (state.KeepRunning()) {
		char* pMem = X_NEW_ARRAY(char, size, &arena, "test");
		X_NAMESPACE(core)::Mem::DeleteArray(pMem, &arena);
	}
}

void BM_mem_blockallocator_arena(benchmark::State& state) {

	const size_t size = state.range(0);

	typedef core::MemoryArena<
		core::GrowingBlockAllocator,
		core::SingleThreadPolicy,
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
	> BlockArena;

	BlockArena::AllocationPolicy allocator;
	BlockArena arena(&allocator, "BenchmarkArena");

	while (state.KeepRunning()) {
		char* pMem = X_NEW_ARRAY(char, size, &arena, "test");
		X_NAMESPACE(core)::Mem::DeleteArray(pMem, &arena);
	}
}

// malloc speeds
BENCHMARK(BM_mem_malloc)->RangeMultiplier(2)->Range(16, 1 << 16);

// test the blockallocator to get base line, then see overhead of various arenas.
BENCHMARK(BM_mem_blockallocator)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_mem_blockallocator_simplearena)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_mem_blockallocator_arena)->RangeMultiplier(2)->Range(16, 1 << 16);

