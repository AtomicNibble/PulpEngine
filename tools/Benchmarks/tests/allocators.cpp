#include "stdafx.h"

#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\MemoryTrackingPolicies\SimpleMemoryTracking.h>
#include <Memory\SimpleMemoryArena.h>


void BM_mem_malloc(benchmark::State& state) {

	const size_t size = state.range(0);

	while (state.KeepRunning()) {
		void* pMem = malloc(size); 
		free(pMem);
	}
}
void BM_mem_malloc_rand(benchmark::State& state) {

	const size_t min = state.range(0);
	const size_t max = state.range(1);

	while (state.KeepRunning()) {
		size_t size = rand() % (max - min) + min;
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

void BM_mem_blockallocator_rand(benchmark::State& state) {

	const size_t min = state.range(0);
	const size_t max = state.range(1);

	core::GrowingBlockAllocator allocator;

	while (state.KeepRunning()) {
		size_t size = rand() % (max - min) + min;
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

template<typename ArenaT>
void BM_mem_blockallocator_arena(benchmark::State& state) {

	const size_t size = state.range(0);

	core::GrowingBlockAllocator allocator;
	ArenaT arena(&allocator, "BenchmarkArena");

	while (state.KeepRunning()) {
		char* pMem = X_NEW_ARRAY(char, size, &arena, "test");
		X_NAMESPACE(core)::Mem::DeleteArray(pMem, &arena);
	}
}


typedef core::MemoryArena<
	core::GrowingBlockAllocator,
	core::SingleThreadPolicy,
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
> BlockArenaST;

typedef core::MemoryArena<
	core::GrowingBlockAllocator,
	core::MultiThreadPolicy<core::Spinlock>,
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
> BlockArenaMT;

// malloc speeds
BENCHMARK(BM_mem_malloc)->RangeMultiplier(2)->Range(16, 1 << 16);


// test the blockallocator to get base line, then see overhead of various arenas.
BENCHMARK(BM_mem_blockallocator)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_mem_blockallocator_simplearena)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK_TEMPLATE(BM_mem_blockallocator_arena, BlockArenaST)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK_TEMPLATE(BM_mem_blockallocator_arena, BlockArenaMT)->RangeMultiplier(2)->Range(16, 1 << 16);

// some tests with random sizes.
BENCHMARK(BM_mem_malloc_rand)->RangePair(1, 255, 256, 1024 * 8);
BENCHMARK(BM_mem_blockallocator_rand)->RangePair(1, 255, 256, 1024 * 8);
