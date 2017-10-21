#pragma once

#include <EngineCommon.h>

#include <Util\UniquePointer.h>

// #define WIN32_LEAN_AND_MEAN            
#include <windows.h>

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
