#pragma once

#include <EngineCommon.h>

#include <Util\UniquePointer.h>

#include <tchar.h>
#include <iostream>
#include <ostream>
#include <fstream>

#define _BOOL

#define MODELEX_EXPORT __declspec(dllexport) 


#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya\MDistance.h>


extern core::MemoryArenaBase* g_arena;
