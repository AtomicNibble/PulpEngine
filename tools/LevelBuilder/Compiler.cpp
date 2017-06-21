#include "stdafx.h"
#include "Compiler.h"

#include <Time\StopWatch.h>

#include <IFileSys.h>
#include <ITimer.h>
#include <IConsole.h>

#include "mapFile\MapFile.h"
#include "LvlBuilder.h"

#include "Model/ModelCache.h"
#include "Material/MaterialManager.h"

X_NAMESPACE_BEGIN(lvl)

core::MemoryArenaBase* g_bspFaceArena = nullptr;
core::MemoryArenaBase* g_bspNodeArena = nullptr;
core::MemoryArenaBase* g_windingArena = nullptr;


Compiler::Compiler(core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooking) :
	arena_(arena),
	pPhysCooking_(pPhysCooking),
	bspFaceAllocator_(sizeof(bspFace), X_ALIGN_OF(bspFace), 1 << 20, core::VirtualMem::GetPageSize() * 8),
	bspNodeAllocator_(sizeof(bspNode), X_ALIGN_OF(bspNode), 1 << 20, core::VirtualMem::GetPageSize() * 8)
{
	pModelCache_ = X_NEW(ModelCache, arena, "LvlModelCache")(arena);
	pMaterialMan_ = X_NEW(MatManager, arena, "LvlMaterialMan")(arena);

	// set the pointers.
	g_bspFaceArena = &bspFaceAllocator_.arena_;
	g_bspNodeArena = &bspNodeAllocator_.arena_;
	g_windingArena = arena_;
}

Compiler::~Compiler()
{
	g_bspFaceArena = nullptr;
	g_bspNodeArena = nullptr;
	g_windingArena = nullptr;

	X_DELETE(pModelCache_, arena_);
	X_DELETE(pMaterialMan_, arena_);
}


bool Compiler::compileLevel(core::Path<char>& path)
{
	if (core::strUtil::IsEqualCaseInsen("map", path.extension()))
	{
		X_ERROR("Map", "extension is not valid, must be .map");
		return false;
	}

	X_LOG0("Map", "Loading: \"%s\"", path.fileName());

	core::StopWatch stopwatch;

	core::XFileMemScoped file;
	if (!file.openFile(path.c_str(), core::IFileSys::fileMode::READ | core::IFileSys::fileMode::SHARE))
	{
		return false;
	}

	mapFile::XMapFile map(g_arena);
	LvlBuilder lvl(pPhysCooking_, g_arena);

	if (!lvl.init())
	{
		X_ERROR("Map", "Failed to init level builder");
		return false;
	}

		//	parse the map file.
	if (!map.Parse(file->getBufferStart(), safe_static_cast<size_t, uint64_t>(file->getSize())))
	{
		X_ERROR("Map", "Failed to parse map file");
		return false;
	}
	
	core::TimeVal elapsed = stopwatch.GetTimeVal();
	{
		X_LOG_BULLET;
		X_LOG0("Map", "Loaded: ^6%.4fms", elapsed.GetMilliSeconds());
		X_LOG0("Map", "Num Entities: ^8%" PRIuS, map.getNumEntities());

		for (uint32_t prim = 0; prim < mapFile::PrimType::ENUM_COUNT; ++prim)
		{
			X_LOG0("Map", "Num %s: ^8%" PRIuS, mapFile::PrimType::ToString(prim), map.getPrimCounts()[prim]);
		}
	}

	// all loaded time to get naked.
	if (!lvl.LoadFromMap(&map))
	{
		return false;
	}

	if (!lvl.ProcessModels())
	{
		return false;
	}

	if (!lvl.save(path.fileName()))
	{
		X_ERROR("Level", "Failed to save: \"%s\"", path.fileName());
		return false;
	}

	elapsed = stopwatch.GetTimeVal();
	X_LOG0("Info", "Total Time: ^6%.4fms", elapsed.GetMilliSeconds());
	return true;
}

X_NAMESPACE_END