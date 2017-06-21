#include "stdafx.h"
#include "Loader.h"

#include "MapTypes.h"
#include "MapFile.h"
#include "Util.h"

#include "Material\MaterialManager.h"

#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(lvl)

namespace mapFile
{
	


	bool MapFileSource::load(core::Path<char>& path)
	{
		core::StopWatch stopwatch;

		core::XFileMemScoped file;
		if (!file.openFile(path.c_str(), core::IFileSys::fileMode::READ | core::IFileSys::fileMode::SHARE))
		{
			return false;
		}

		XMapFile map(arena_);
		if (!map.Parse(file->getBufferStart(), safe_static_cast<size_t>(file->getSize())))
		{
			X_ERROR("Map", "Failed to parse map file");
			return false;
		}

		// process it.
		if (map.getNumEntities() == 0) {
			X_ERROR("Lvl", "Map has zero entites, atleast one is required");
			return false;
		}

		entities_.resize(map.getNumEntities());
		for (size_t i = 0; i < map.getNumEntities(); i++)
		{
			if (!processMapEntity(entities_[i], map.getEntity(i))) {
				X_ERROR("Lvl", "Failed to process entity: %" PRIuS, i);
				return false;
			}
		}

		// calculate bouds.
		calculateLvlBounds();

		AABB::StrBuf boundsStr;
		X_LOG0("Map", "Total world brush: ^8%" PRIuS, entities_[0].brushes.size());
		X_LOG0("Map", "Total brush: ^8%" PRIi32, stats_.numBrushes);
		X_LOG0("Map", "Total patches: ^8%" PRIi32, stats_.numPatches);
		X_LOG0("Map", "Total entities: ^8%" PRIi32, stats_.numEntities);
		X_LOG0("Map", "Size: %s", mapBounds_.toString(boundsStr));

		return true;
	}

	bool MapFileSource::processMapEntity(LvlEntity& ent, XMapEntity* mapEnt)
	{
		// update stats.
		stats_.numEntities++;

		// the map ent this LvlEnt is made from.
		ent.pMapEntity = mapEnt;

		// ensure we never resize, otherwise originals pointers fuck up.
		ent.brushes.reserve(mapEnt->GetNumPrimitives());

		// we process brushes / patches diffrent.
		for (size_t i = 0; i < mapEnt->GetNumPrimitives(); i++)
		{
			auto* pPrim = mapEnt->GetPrimitive(i);

			if (pPrim->getType() == PrimType::BRUSH) {
				++stats_.numBrushes;

				if (!processBrush(ent, *static_cast<XMapBrush*>(pPrim), i)) {
					X_ERROR("Lvl", "failed to process brush: %" PRIuS, i);
					return false;
				}
			}
			else if (pPrim->getType() == PrimType::PATCH) {
				++stats_.numPatches;

				if (!processPatch(ent, *static_cast<XMapPatch*>(pPrim), i)) {
					X_ERROR("Lvl", "failed to process patch: %" PRIuS, i);
					return false;
				}
			}
			else {
				X_ASSERT_NOT_IMPLEMENTED();
			}
		}

		auto it = mapEnt->epairs.find(X_CONST_STRING("origin"));
		if (it != mapEnt->epairs.end())
		{
			// set the origin.
			const core::string& value = it->second;
			if (sscanf_s(value.c_str(), "%f %f %f", &ent.origin.x, &ent.origin.y, &ent.origin.z) != 3)
			{
				return false;
			}
		}

		// check for angles.
		it = mapEnt->epairs.find(X_CONST_STRING("angles"));
		if (it != mapEnt->epairs.end())
		{
			const core::string& value = it->second;
			if (sscanf_s(value.c_str(), "%f %f %f", &ent.angle.x, &ent.angle.y, &ent.angle.z) != 3)
			{
				return false;
			}
		}

		// get classname.
		it = mapEnt->epairs.find(X_CONST_STRING("classname"));
		if (it != mapEnt->epairs.end())
		{
			core::string& classname = it->second;

			if (classname == "worldspawn")
			{
				ent.classType = level::ClassType::WORLDSPAWN;
			}
			else if (classname == "misc_model")
			{
				ent.classType = level::ClassType::MISC_MODEL;
			}
			else if (classname == "info_player_start")
			{
				ent.classType = level::ClassType::PLAYER_START;
			}
			else if (classname == "func_group")
			{
				ent.classType = level::ClassType::FUNC_GROUP;
			}
			else
			{
				X_WARNING("Lvl", "ent has unknown class type: \"%s\"", classname.c_str());
			}
		}
		else
		{
			X_WARNING("Lvl", "ent missing class type");
		}

		if (ent.classType == level::ClassType::MISC_MODEL)
		{
			it = mapEnt->epairs.find(X_CONST_STRING("model"));
			if (it == mapEnt->epairs.end())
			{
				X_ERROR("Lvl", "Ent with classname \"misc_model\" is missing \"model\" kvp at (%g,%g,%g)",
					ent.origin.x, ent.origin.y, ent.origin.z);
				return false;
			}
		}

		return true;
	}

	bool MapFileSource::processBrush(LvlEntity& ent, XMapBrush& mapBrush, size_t entIdx)
	{
		LvlBrush& brush = ent.brushes.AddOne();
		brush.entityNum = safe_static_cast<int32_t>(stats_.numEntities);
		brush.brushNum = safe_static_cast<int32_t>(entIdx);

		size_t numSides = mapBrush.GetNumSides();
		for (size_t i = 0; i < numSides; i++)
		{
			LvlBrushSide& side = brush.sides.AddOne();
			auto* pMapBrushSide = mapBrush.GetSide(i);

			side.planenum = findFloatPlane(pMapBrushSide->GetPlane());
			// material
			const auto& sideMat = pMapBrushSide->GetMaterial();
			side.matInfo.name = sideMat.name;
			side.matInfo.matRepeate = sideMat.matRepeate;
			side.matInfo.rotate = sideMat.rotate;
			side.matInfo.shift = sideMat.shift;

			// load the material.
			side.matInfo.pMaterial = matMan_.loadMaterial(pMapBrushSide->GetMaterialName());
			if (!side.matInfo.pMaterial->isLoaded()) {
				X_ERROR("Brush", "Failed to load material for brush side \"%s\"", pMapBrushSide->GetMaterialName());
				return false;
			}
		}

		if (!brush.removeDuplicateBrushPlanes()) {
			X_ERROR("Brush", "Failed to remove duplicate planes");
			return false;
		}

		if (!brush.calculateContents()) {
			X_ERROR("Brush", "Failed to calculate brush contents");
			return false;
		}

		// create windings for sides + bounds for brush
		if (!brush.createBrushWindings(planes_)) {
			X_ERROR("Brush", "Failed to create windings for brush");
			return false;
		}

		// set original.
		brush.pOriginal = &brush;


		// check if we have a portal.
#if 0
		if (brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL)) {
			stats_.numAreaPortals++;
		}
#endif

		for (size_t i = 0; i < brush.sides.size(); i++)
		{
			const LvlBrushSide& side = brush.sides[i];
			auto* pWinding = side.pWinding;

			if (!pWinding) {
				continue;
			}

			auto* pMapBrushSide = mapBrush.GetSide(i);
			const Planef& plane = pMapBrushSide->GetPlane();
			const auto mat = pMapBrushSide->GetMaterial();
			const Vec2f& repeate = mat.matRepeate;
			const Vec2f& shift = mat.shift;
			const float& rotate = mat.rotate;

			Vec4f mappingVecs[2];
			QuakeTextureVecs(plane, shift, rotate, repeate, mappingVecs);

			for (size_t j = 0; j < pWinding->getNumPoints(); j++)
			{
				// gets me position from 0,0 from 2d plane.
				Vec5f& point = pWinding->operator[](j);
				Vec3f translated(point.asVec3() + ent.origin);

				point.s = mappingVecs[0][3] + mappingVecs[0].dot(translated);
				point.t = mappingVecs[1][3] + mappingVecs[1].dot(translated);
			}
		}

		return true;
	}



	bool MapFileSource::processPatch(LvlEntity& ent, XMapPatch& patch, size_t entIdx)
	{
		X_UNUSED(entIdx);

		if (gSettings.noPatches) { // are these goat meshes even allowed O_0 ?
			return false;
		}

		// meshes not supported yet.
		//	if (mapBrush->isMesh()) {
		//		return false;
		//	}

		if (patch.isMesh()) {
			patch.CreateNormalsAndIndexes();
		}
		else {
			patch.Subdivide(DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR,
				DEFAULT_CURVE_MAX_LENGTH, true);
		}

		engine::Material* pMaterial = matMan_.loadMaterial(patch.GetMatName());

		X_ASSERT_NOT_NULL(pMaterial);
		// this code seams to expect material to always load?
		// maybe thats just incorrect logic.
		X_ASSERT(pMaterial->isLoaded(), "Material should be loaded?")();

		// create a Primative
		for (size_t i = 0; i < patch.GetNumIndexes(); i += 3)
		{
			LvlTris& tri = ent.patches.AddOne();

			tri.pMaterial = pMaterial;
			tri.verts[2] = patch[patch.GetIndexes()[i + 0]];
			tri.verts[1] = patch[patch.GetIndexes()[i + 2]];
			tri.verts[0] = patch[patch.GetIndexes()[i + 1]];
		}

		return true;
	}


} // namespace mapFile


X_NAMESPACE_END