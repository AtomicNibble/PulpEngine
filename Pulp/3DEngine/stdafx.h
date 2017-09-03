#pragma once



#include <EngineCommon.h>
#include <IAsyncLoad.h>

#include <array>

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
// Model Lib
#include <../../tools/ModelLib/ModelLib.h>

X_LINK_LIB("engine_ImgLib");
X_LINK_LIB("engine_MaterialLib");
X_LINK_LIB("engine_ModelLib");
X_LINK_LIB("engine_AnimLib");


extern core::MemoryArenaBase* g_3dEngineArena;

#include "Util\Config.h"


#include "EngineEnv.h"