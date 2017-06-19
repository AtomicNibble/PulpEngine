#pragma once

#ifndef X_MAP_FILE_LOADER_H_
#define X_MAP_FILE_LOADER_H_

#include "MapTypes.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\AllocationPolicies\MallocFreeAllocator.h>
#include <Containers\Array.h>

X_NAMESPACE_BEGIN(mapfile)

// using the pool gives about a 75ms speed up on avg.
// at the cost of more memory usage.

typedef core::MemoryArena<
	core::GrowingPoolAllocator,
	core::SingleThreadPolicy,
	core::NoBoundsChecking,

#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleMemoryTracking,
	//	core::NoMemoryTracking,
#else
	core::NoMemoryTracking,
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING


	core::NoMemoryTagging>
PrimativePoolArena;


template<typename ArenaType>
struct GrowingPool
{
	GrowingPool(size_t maxAllocsize, size_t maxAlign, size_t maxAllocs, size_t growSize)  :
		allocator_(
			core::bitUtil::NextPowerOfTwo(
				ArenaType::getMemoryRequirement(maxAllocsize) * maxAllocs
			),
			core::bitUtil::NextPowerOfTwo(
				ArenaType::getMemoryRequirement(growSize)
			),
			0,
			ArenaType::getMemoryRequirement(maxAllocsize),
			ArenaType::getMemoryAlignmentRequirement(maxAlign),
			ArenaType::getMemoryOffsetRequirement()
		),
		arena_(&allocator_, "PoolArena")
	{
	}

	core::GrowingPoolAllocator	allocator_;
	ArenaType					arena_;
};


class XMapFile
{
	typedef core::Array<XMapEntity*> EntityArray;
	typedef core::Array<Layer> LayerArray;
	typedef std::array<size_t, PrimType::ENUM_COUNT> PrimTypeNumArr;

public:
	XMapFile(core::MemoryArenaBase* arena);
	~XMapFile();

	bool Parse(const char* pData, size_t length);

	X_INLINE size_t getNumEntities(void) const { return entities_.size(); }
	X_INLINE XMapEntity* getEntity(size_t i) const { return entities_[i]; }

	X_INLINE const PrimTypeNumArr& getPrimCounts(void) const { return primCounts_; }

private:
	IgnoreList getIgnoreList(void) const;
	bool isLayerIgnored(const core::string& layerName) const;
	void ListLayers(void) const;

	void PrimtPrimMemInfo(void) const;

private:
	GrowingPool<PrimativePoolArena> pool_;
	
	core::MemoryArenaBase* arena_;

	EntityArray	entities_;
	LayerArray layers_;
	PrimTypeNumArr primCounts_;
};

X_NAMESPACE_END

#endif // X_MAP_FILE_LOADER_H_