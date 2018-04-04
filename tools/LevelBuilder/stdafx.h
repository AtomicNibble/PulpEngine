#pragma once

#define _LAUNCHER

#include <EngineCommon.h>

#include "resource.h"
#include <tchar.h>
#include <array>


#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

#include <String\Lexer.h>
#include <String\StrRef.h>

#include <Math\XAabb.h>
#include <Math\XWinding.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>
#include <Containers\FixedArray.h>

#include <Util\UniquePointer.h>

#include "../../tools/MaterialLib/MatLib.h"
#include "../../tools/ModelLib/ModelLib.h"


#include <Ilevel.h>
#include <IPhysics.h>
#include <IModel.h>
#include <ILevel.h>

typedef core::StackString<level::MAP_MAX_MATERIAL_LEN> MaterialName;

#include "Util/Config.h"
#include "Util/Util.h"
#include "Util/PlaneSet.h"
#include "Util/KvpMap.h"
#include "Settings.h"
#include "LvlVert.h"



X_NAMESPACE_BEGIN(level)


extern core::MemoryArenaBase* g_arena;
extern core::MemoryArenaBase* g_bspFaceArena;
extern core::MemoryArenaBase* g_bspPortalArena;
extern core::MemoryArenaBase* g_bspNodeArena;
extern core::MemoryArenaBase* g_windingArena;
extern core::MemoryArenaBase* g_windingPointsArena;


class WindingAlloc
{
public:
	X_INLINE Vec5f* alloc(size_t num)
	{
		return X_NEW_ARRAY(Vec5f, num, g_windingPointsArena, "WindingRealoc");
	}

	X_INLINE void free(Vec5f* pPoints)
	{
		X_DELETE_ARRAY(pPoints, g_windingPointsArena);
	}
};

typedef XWindingT<WindingAlloc> Winding;


X_NAMESPACE_END