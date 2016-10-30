#pragma once

#define _LAUNCHER

#include <EngineCommon.h>

#include "resource.h"

#include <String\Lexer.h>
#include <String\StrRef.h>

#include <Math\XAabb.h>
#include <Math\XWinding.h>

#include <Containers\HashMap.h>

#include "StringPair.h"


#include <Ilevel.h>

#include "../../tools/MaterialLib/MatLib.h"
#include "../../tools/ModelLib/ModelLib.h"

X_USING_NAMESPACE;


// #include <vector>
// #include <map>

// using namespace std;

#include "Globals.h"
#include "Settings.h"
#include "PlaneSet.h"
#include "VertexFmt.h"


extern core::MemoryArenaBase* g_arena;
// could combine these two, but be quite a bit of waste for faces.
extern core::MemoryArenaBase* g_bspFaceAllocator;
extern core::MemoryArenaBase* g_bspNodeAllocator;
