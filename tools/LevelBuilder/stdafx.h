#pragma once

#define _LAUNCHER

#include <EngineCommon.h>

#include "resource.h"
#include <tchar.h>


#include <String\Lexer.h>
#include <String\StrRef.h>

#include <Math\XAabb.h>
#include <Math\XWinding.h>

#include <Containers\HashMap.h>
#include <Containers\FixedArray.h>

#include <Util\UniquePointer.h>

#include "StringPair.h"


#include <Ilevel.h>

#include "../../tools/MaterialLib/MatLib.h"
#include "../../tools/ModelLib/ModelLib.h"

X_USING_NAMESPACE;

#include <IPhysics.h>
#include <IModel.h>
#include <ILevel.h>

typedef core::StackString<level::MAP_MAX_MATERIAL_LEN> MaterialName;

#include "Globals.h"
#include "Settings.h"
#include "PlaneSet.h"
#include "VertexFmt.h"


extern core::MemoryArenaBase* g_arena;
// could combine these two, but be quite a bit of waste for faces.
extern core::MemoryArenaBase* g_bspFaceArena;
extern core::MemoryArenaBase* g_bspNodeArena;
extern core::MemoryArenaBase* g_windingArena;
