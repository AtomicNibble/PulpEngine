#pragma once

#include <EngineCommon.h>

#include "resource.h"

#include <String\StackString.h>
#include <String\Path.h>
#include <String\Lexer.h>
#include <String\StackString.h>
#include <String\StrRef.h>

#include <Math\XAabb.h>

#include <Containers\HashMap.h>

#include "StringPair.h"


#include <Ilevel.h>

X_USING_NAMESPACE;


#include <vector>
#include <map>

using namespace std;

#include "Globals.h"
#include "Settings.h"
#include "Winding.h"
#include "PlaneSet.h"
#include "VertexFmt.h"


extern core::MemoryArenaBase* g_arena;
extern core::MemoryArenaBase* g_bspFaceAllocator;
