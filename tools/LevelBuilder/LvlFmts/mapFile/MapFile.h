#pragma once

#ifndef X_MAP_FILE_LOADER_H_
#define X_MAP_FILE_LOADER_H_

#include "MapTypes.h"
#include <Containers\Array.h>
#include "Util/GrowingPool.h"

X_NAMESPACE_BEGIN(level)

namespace mapFile
{

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
		core::NoMemoryTagging
	>
		PrimativePoolArena;

class XMapFile
{
	typedef core::Array<XMapEntity*> EntityArray;
	typedef core::Array<Layer> LayerArray;
	typedef std::array<size_t, PrimType::ENUM_COUNT> PrimTypeNumArr;

public:
	XMapFile(core::MemoryArenaBase* arena);
	~XMapFile();

	bool Parse(const char* pData, size_t length);
	void PrimtPrimMemInfo(void) const;

	X_INLINE size_t getNumEntities(void) const;
	X_INLINE XMapEntity* getEntity(size_t i) const;

	X_INLINE const PrimTypeNumArr& getPrimCounts(void) const;

private:
	IgnoreList getIgnoreList(void) const;
	bool isLayerIgnored(const core::string& layerName) const;
	void ListLayers(void) const;


private:
	GrowingPool<PrimativePoolArena> pool_;
	
	core::MemoryArenaBase* arena_;

	EntityArray	entities_;
	LayerArray layers_;
	PrimTypeNumArr primCounts_;
};


} // namespae mapFile

X_NAMESPACE_END

#include "MapFile.inl"

#endif // X_MAP_FILE_LOADER_H_