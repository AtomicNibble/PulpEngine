#pragma once



#include <EngineCommon.h>

#include <Math\XMatrixAlgo.h>

#define IPRENDERSYS_EXPORTS

#include <IMaterial.h>
#include "Material\MaterialManager.h"


// Img Lib
#include <../../tools/ImgLib/ImgLib.h>
// Mat Lib
#include <../../tools/MaterialLib/MatLib.h>

X_LINK_LIB("engine_ImgLib");
X_LINK_LIB("engine_MaterialLib");


extern core::MemoryArenaBase* g_3dEngineArena;

#include "Util\Config.h"
