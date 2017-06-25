#pragma once


#include "Util\GrowingPool.h"

X_NAMESPACE_BEGIN(lvl)

class ModelCache;
class MatManager;

class Compiler
{
	typedef core::MemoryArena<
		core::GrowingPoolAllocator,
		core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> PoolArena;

public:
	Compiler(core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooking);
	~Compiler();

	bool compileLevel(core::Path<char>& path);

private:
	core::MemoryArenaBase* arena_;
	physics::IPhysicsCooking* pPhysCooking_;
	ModelCache* pModelCache_;
	MatManager* pMaterialMan_;

	GrowingPool<PoolArena> bspFaceAllocator_;
	GrowingPool<PoolArena> bspPortalAllocator_;
	GrowingPool<PoolArena> bspNodeAllocator_;
};


X_NAMESPACE_END