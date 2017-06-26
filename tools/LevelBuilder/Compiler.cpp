#include "stdafx.h"
#include "Compiler.h"

#include <Time\StopWatch.h>

#include <IFileSys.h>
#include <ITimer.h>
#include <IConsole.h>

#include "LvlFmts\LvlSource.h"
#include "LvlFmts/mapFile\MapFile.h"
#include "LvlFmts/mapFile/Loader.h"
#include "LvlBuilder.h"

#include "Model/ModelCache.h"
#include "Material/MaterialManager.h"

X_NAMESPACE_BEGIN(lvl)

core::MemoryArenaBase* g_bspFaceArena = nullptr;
core::MemoryArenaBase* g_bspPortalArena = nullptr;
core::MemoryArenaBase* g_bspNodeArena = nullptr;
core::MemoryArenaBase* g_windingArena = nullptr;
core::MemoryArenaBase* g_windingPointsArena = nullptr;


Compiler::Compiler(core::MemoryArenaBase* arena, physics::IPhysicsCooking* pPhysCooking) :
	arena_(arena),
	planes_(arena),
	pPhysCooking_(X_ASSERT_NOT_NULL(pPhysCooking)),
	bspFaceAllocator_(sizeof(bspFace), X_ALIGN_OF(bspFace), 1 << 20, core::VirtualMem::GetPageSize() * 8), // grow 32k at a time.
	bspPortalAllocator_(core::Max(sizeof(bspPortal), sizeof(Winding)), core::Max(X_ALIGN_OF(bspPortal), X_ALIGN_OF(Winding)), 1 << 20, core::VirtualMem::GetPageSize() * 8),
	bspNodeAllocator_(sizeof(bspNode), X_ALIGN_OF(bspNode), 1 << 20, core::VirtualMem::GetPageSize() * 8),
	windingDataAllocator_((1 << 20) * 10, core::VirtualMem::GetPageSize() * 8, 16, WindingDataArena::getMemoryOffsetRequirement() + 4),
	windingDataArena_(&windingDataAllocator_, "WindingDataArena"),

	areas_(arena),
	staticModels_(arena),
	multiRefEntLists_{
		X_PP_REPEAT_COMMA_SEP( 8, arena)
	},
	multiModelRefLists_{
		X_PP_REPEAT_COMMA_SEP(8, arena)
	},
	stringTable_(arena)
{
	pModelCache_ = X_NEW(ModelCache, arena, "LvlModelCache")(arena);
	pMaterialMan_ = X_NEW(MatManager, arena, "LvlMaterialMan")(arena);

	// set the pointers.
	g_bspFaceArena = &bspFaceAllocator_.arena_;
	g_bspPortalArena = &bspPortalAllocator_.arena_;
	g_bspNodeArena = &bspNodeAllocator_.arena_;
	g_windingArena = &bspPortalAllocator_.arena_;
	g_windingPointsArena = &windingDataArena_;
}

Compiler::~Compiler()
{
	g_bspFaceArena = nullptr;
	g_bspPortalArena = nullptr;
	g_bspNodeArena = nullptr;
	g_windingArena = nullptr;
	g_windingPointsArena = nullptr;

	if (pMaterialMan_) {
		pMaterialMan_->ShutDown();
	}

	X_DELETE(pModelCache_, arena_);
	X_DELETE(pMaterialMan_, arena_);
}



bool Compiler::init(void)
{
	if (!pMaterialMan_->Init()) {
		return false;
	}

	if (!pModelCache_->loadDefaultModel()) {
		return false;
	}

	return true;
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

bool Compiler::processModels(LvlEntsArr& ents)
{
	for (size_t i = 0; i < ents.size(); i++)
	{
		LvlEntity& entity = ents[i];
		if (entity.brushes.isEmpty()) {
			continue;
		}

		X_LOG0("Entity", "^5processing entity %" PRIuS, i);

		if (i == 0)
		{
			// return false if leak.
			if (!processWorldModel(ents, entity)) {
				return false;
			}
		}
		else
		{
			if (!processModel(entity)) {
				X_ERROR("Entity", "Failed to process entity: %" PRIuS, i);

				return false;
			}
		}
	}

	return true;
}

bool Compiler::processModel(LvlEntity& ent)
{
	X_ASSERT_NOT_IMPLEMENTED();
	return false;
}

bool Compiler::processWorldModel(LvlEntsArr& ents, LvlEntity& ent)
{
	if (ent.classType != level::ClassType::WORLDSPAWN) {
		X_ERROR("Lvl", "World model is missing class name: 'worldspawn'");
		return false;
	}

	if (!ent.MakeStructuralFaceList()) {
		return false;
	}

	if (!ent.FacesToBSP(planes_)) {
		return false;
	}
	
	if (!ent.MakeTreePortals(planes_)) {
		return false;
	}
	
	if (!ent.FilterBrushesIntoTree(planes_)) {
		return false;
	}

	if (!ent.FloodEntities(planes_, ents)) {
		X_ERROR("Lvl", "leaked");
		return false;
	}
	
	if (!ent.FillOutside()) {
		return false;
	}

	if (!ent.ClipSidesByTree(planes_)) {
		return false;
	}

	if (!ent.FloodAreas()) {
		return false;
	}

	if (!createAreasForPrimativates(ent)) {
		return false;
	}

	if (!ent.PruneNodes()) {
		return false;
	}

//	if (!CreateEntAreaRefs(ent)) {
//		return false;
//	}

	return true;
}


bool Compiler::createAreasForPrimativates(LvlEntity& ent)
{
	X_LOG0("Lvl", "^5addEntsPrimativesToAreas");

	if (ent.numAreas < 1) {
		X_ERROR("Lvl", "Ent has no areas");
		return false;
	}

	areas_.clear();
	areas_.resize(ent.numAreas);

	for (auto& area : areas_) {
		area.AreaBegin();
	}

	for (size_t i = 0; i < ent.brushes.size(); i++)
	{
		LvlBrush& brush = ent.brushes[i];
		// for each side that's visable.

		for (size_t j = 0; j < brush.sides.size(); j++)
		{
			LvlBrushSide& side = brush.sides[j];
			if (!side.pVisibleHull) {
				continue;
			}

			auto* pMaterial = side.matInfo.pMaterial;
			if (!pMaterial) {
				X_WARNING("Lvl", "side without a material");
				continue;
			}

			// skip none visable materials.
			if (!pMaterial->isDrawn()) {
				X_LOG1("Lvl", "Skipping visible face, material not drawn: \"%s\"", side.matInfo.name.c_str());
				continue;
			}

			putWindingIntoAreas_r(side.pVisibleHull, side, ent.bspTree_.pHeadnode);
		}
	}

	for (size_t i = 0; i < ent.patches.size(); i++)
	{
		LvlTris& tris = ent.patches[i];

		// AddMapTriToAreas(ent, planes_, tris);
	}

	for (size_t i = 0; i < areas_.size(); i++) {
		areas_[i].AreaEnd();

		if (!areas_[i].model.BelowLimits()) {
			X_ERROR("Lvl", "Area %" PRIuS " exceeds the limits", i);
			return false;
		}
	}

	return true;
}


void Compiler::putWindingIntoAreas_r(Winding* pWinding, LvlBrushSide& side, bspNode* pNode)
{
	if (!pWinding) {
		return;
	}

	if (pNode->planenum != PLANENUM_LEAF)
	{
		if (side.planenum == pNode->planenum) {
			putWindingIntoAreas_r(pWinding, side, pNode->children[Side::FRONT]);
			return;
		}
		if (side.planenum == (pNode->planenum ^ 1)) {
			putWindingIntoAreas_r(pWinding, side, pNode->children[Side::BACK]);
			return;
		}

		Winding *pFront, *pBack;

		pWinding->Split(planes_[pNode->planenum], ON_EPSILON, &pFront, &pBack, g_windingArena);

		putWindingIntoAreas_r(pFront, side, pNode->children[Side::FRONT]);
		putWindingIntoAreas_r(pBack, side, pNode->children[Side::BACK]);

		if (pFront) {
			X_DELETE(pFront, g_windingArena);
		}
		if (pBack) {
			X_DELETE(pBack, g_windingArena);
		}

		return;
	}

	// if opaque leaf,  don't add
	if (pNode->opaque) {
		return;
	}

	X_ASSERT(pNode->area != -1 && pNode->area < areas_.size(), "Leaf node has invalid area")(pNode->area);

	LvlArea& area = areas_[pNode->area];	
	area.addWindingForSide(planes_, side, pWinding);
}

X_NAMESPACE_END