#pragma once


#include "Util\GrowingPool.h"
#include <Memory\SimpleMemoryArena.h>
#include <Memory\AllocationPolicies\GrowingGenericAllocator.h>

#include "LvlEntity.h"
#include "LvlArea.h"

X_NAMESPACE_BEGIN(lvl)

class ModelCache;
class MatManager;

class Compiler
{
#if X_ENABLE_MEMORY_DEBUG_POLICIES

	typedef core::MemoryArena<
		core::GrowingGenericAllocator,
		core::SingleThreadPolicy,
		core::SimpleBoundsChecking,
		// core::SimpleMemoryTracking,
		core::NoMemoryTracking, 
		core::SimpleMemoryTagging
	> WindingDataArena;

	typedef core::MemoryArena<
		core::GrowingPoolAllocator,
		core::SingleThreadPolicy,
		core::SimpleBoundsChecking,
		// core::SimpleMemoryTracking,
		core::NoMemoryTracking,
		core::SimpleMemoryTagging
	> PoolArena;
#else

	typedef core::SimpleMemoryArena<
		core::GrowingGenericAllocator
	> WindingDataArena;

	typedef core::SimpleMemoryArena<
		core::GrowingPoolAllocator
	> PoolArena;


#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING

	typedef core::Array<LvlEntity> LvlEntsArr;
	typedef core::Array<LvlArea> LvlAreaArr;
	typedef core::Array<level::FileStaticModel> StaticModelsArr;
	typedef std::array<core::Array<level::MultiAreaEntRef>,
		level::MAP_MAX_MULTI_REF_LISTS> MultiRefArr;

public:
	Compiler(core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooking);
	~Compiler();

	bool init(void);

	bool compileLevel(core::Path<char>& path);

private:
	bool processModels(LvlEntsArr& ents);
	bool processModel(LvlEntity& ent);
	bool processWorldModel(LvlEntsArr& ents, LvlEntity& ent);

	bool createAreasForPrimativates(LvlEntity& ent);
	void putWindingIntoAreas_r(Winding* pWinding, LvlBrushSide& side, bspNode* pNode);

private:
	core::MemoryArenaBase* arena_;
	physics::IPhysicsCooking* pPhysCooking_;
	ModelCache* pModelCache_;
	MatManager* pMaterialMan_;

	GrowingPool<PoolArena> bspFaceAllocator_;
	GrowingPool<PoolArena> bspPortalAllocator_;
	GrowingPool<PoolArena> bspNodeAllocator_;

	core::GrowingGenericAllocator windingDataAllocator_;
	WindingDataArena windingDataArena_;

	XPlaneSet	planes_;

	// Compiled data
	LvlAreaArr	areas_;

	StaticModelsArr staticModels_;

	MultiRefArr multiRefEntLists_;
	MultiRefArr multiModelRefLists_;

	StringTableType stringTable_;
};


X_NAMESPACE_END