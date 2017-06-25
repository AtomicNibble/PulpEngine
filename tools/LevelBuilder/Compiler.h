#pragma once


#include "Util\GrowingPool.h"
#include <Memory\SimpleMemoryArena.h>
#include <Memory\AllocationPolicies\GrowingGenericAllocator.h>

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



public:
	Compiler(core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooking);
	~Compiler();

	bool init(void);

	bool compileLevel(core::Path<char>& path);

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
};


X_NAMESPACE_END