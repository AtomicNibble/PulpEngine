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



X_NAMESPACE_BEGIN(lvl)


extern core::MemoryArenaBase* g_arena;
// could combine these two, but be quite a bit of waste for faces.
extern core::MemoryArenaBase* g_bspFaceArena;
extern core::MemoryArenaBase* g_bspNodeArena;
extern core::MemoryArenaBase* g_windingArena;

X_NAMESPACE_END