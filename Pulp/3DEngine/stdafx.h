#pragma once



#include <EngineCommon.h>

#include <Math\XMatrixAlgo.h>

#include <Memory\MemoryArena.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\VirtualMem.h>

#include <Containers\Array.h>

#define IPRENDERSYS_EXPORTS

#include <IMaterial.h>
#include <IDirectoryWatcher.h>


// Img Lib
#include <../../tools/ImgLib/ImgLib.h>
// Mat Lib
#include <../../tools/MaterialLib/MatLib.h>

X_LINK_LIB("engine_ImgLib");
X_LINK_LIB("engine_MaterialLib");


extern core::MemoryArenaBase* g_3dEngineArena;

#include "Util\Config.h"


#include "EngineEnv.h"