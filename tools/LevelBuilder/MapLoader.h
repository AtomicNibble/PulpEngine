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

#define MAP_LOADER_USE_POOL 1

typedef core::MemoryArena<
#if MAP_LOADER_USE_POOL
	core::GrowingPoolAllocator,
#else
	core::MallocFreeAllocator,
#endif
	core::SingleThreadPolicy,
	core::NoBoundsChecking,
//	core::SimpleMemoryTracking,
	core::NoMemoryTracking,
	core::NoMemoryTagging>
	PrimativePoolArena;

class XMapFile
{
	typedef core::Array<XMapEntity*> EntityArray;
	typedef core::Array<Layer> LayerArray;

public:
	XMapFile();
	~XMapFile();

	bool Parse(const char* pData, size_t length);

	size_t getNumEntities(void) const { return entities_.size(); }
	size_t getNumBrushes(void) const { return numBrushes_; }
	size_t getNumPatches(void) const { return numPatches_; }

	XMapEntity* getEntity(size_t i) const { return entities_[i]; }

private:
	IgnoreList getIgnoreList(void) const;
	bool isLayerIgnored(const core::string& layerName) const;
	void ListLayers(void) const;

	void PrimtPrimMemInfo(void) const;

private:
#if MAP_LOADER_USE_POOL
	core::GrowingPoolAllocator	primPoolAllocator_;
#else
	core::MallocFreeAllocator primAllocator_;
#endif // !MAP_LOADER_USE_POOL
	PrimativePoolArena		primPoolArena_;

	EntityArray	entities_;
	LayerArray layers_;

	size_t numBrushes_;
	size_t numPatches_;
};

X_NAMESPACE_END

#endif // X_MAP_FILE_LOADER_H_