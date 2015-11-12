#include "stdafx.h"
#include "LvlBuilder.h"

#include "MapTypes.h"
#include "MapLoader.h"
#include "ModelInfo.h"


namespace
{

	static void ComputeAxisBase(Vec3f normal, Vec3f& texS, Vec3f& texT)
	{
		float RotY, RotZ;
		// do some cleaning
		if (fabs(normal[0]) < 1e-6) {
			normal[0] = 0.0f;
		}
		if (fabs(normal[1]) < 1e-6) {
			normal[1] = 0.0f;
		}
		if (fabs(normal[2]) < 1e-6) {
			normal[2] = 0.0f;
		}
		RotY = -atan2(normal[2], sqrt(normal[1] * normal[1] + normal[0] * normal[0]));
		RotZ = atan2(normal[1], normal[0]);
		// rotate (0,1,0) and (0,0,1) to compute texS and texT
		texS[0] = -sin(RotZ);
		texS[1] = cos(RotZ);
		texS[2] = 0;
		// the texT vector is along -Z ( T texture coorinates axis )
		texT[0] = -sin(RotY) * cos(RotZ);
		texT[1] = -sin(RotY) * sin(RotZ);
		texT[2] = -cos(RotY);
	}

	static void ConvertTexMatWithQTexture(Vec3f texMat1[2], Vec3f texMat2[2])
	{
		float s1, s2;
		s1 = (512.0f) / (512.0f);
		s2 = (512.0f) / (512.0f);
		texMat2[0][0] = s1 * texMat1[0][0];
		texMat2[0][1] = s1 * texMat1[0][1];
		texMat2[0][2] = s1 * texMat1[0][2];
		texMat2[1][0] = s2 * texMat1[1][0];
		texMat2[1][1] = s2 * texMat1[1][1];
		texMat2[1][2] = s2 * texMat1[1][2];
	}

}


LvlBuilder::LvlBuilder() :
staticModels_(g_arena),
entities_(g_arena),
areas_(g_arena),

multiRefEntLists_({{ g_arena, g_arena, g_arena, g_arena,
g_arena, g_arena, g_arena, g_arena }}),

multiModelRefLists_({ { g_arena, g_arena, g_arena, g_arena,
	g_arena, g_arena, g_arena, g_arena } }),

stringTable_(g_arena),
map_(nullptr)
{
	core::zero_object(stats_);

	matMan_.Init();
}

LvlBuilder::~LvlBuilder()
{
	matMan_.ShutDown();
}



bool LvlBuilder::LoadFromMap(mapfile::XMapFile* map)
{
	X_ASSERT_NOT_NULL(map);
	int32_t i;

	map_ = map;

	// first we need to load the AABB of the default model.
	if (!LoadDefaultModel()) {
		return false;
	}

	if (map->getNumEntities() == 0) {
		X_ERROR("Lvl", "Map has zero entites, atleast one is required");
		return false;
	}

	entities_.resize(map->getNumEntities());
	for (i = 0; i < map->getNumEntities(); i++)
	{
		if (!processMapEntity(entities_[i], map->getEntity(i))) {
			X_ERROR("Lvl", "Failed to process entity: %i", i);
			return false;
		}
	}

	// calculate bouds.
	calculateLvlBounds();


	X_LOG0("Map", "Total world brush: ^8%i", entities_[0].brushes.size());
	X_LOG0("Map", "Total world patches: ^8%i", entities_[0].patches.size()); // TODO
	X_LOG0("Map", "Total total brush: ^8%i", stats_.numBrushes);
	X_LOG0("Map", "Total total patches: ^8%i", stats_.numPatches);
	X_LOG0("Map", "Total entities: ^8%i", stats_.numEntities);
	X_LOG0("Map", "Total planes: ^8%i", this->planes.size());
	X_LOG0("Map", "Total areaPortals: ^8%i", stats_.numAreaPortals);
	X_LOG0("Map", "Size: (^8%.0f,%.0f,%.0f^7) to (^8%.0f,%.0f,%.0f^7)", 
		mapBounds.min[0], mapBounds.min[1], mapBounds.min[2],
		mapBounds.max[0], mapBounds.max[1], mapBounds.max[2]);

	return true;
}

int32_t LvlBuilder::FindFloatPlane(const Planef& plane)
{
	return planes.FindPlane(plane, PLANE_NORMAL_EPSILON, PLANE_DIST_EPSILON);
}

bool LvlBuilder::processMapEntity(LvlEntity& ent, mapfile::XMapEntity* mapEnt)
{
	mapfile::XMapPrimitive* prim;
	int32_t i;

	// update stats.
	stats_.numEntities++;

	// the map ent this LvlEnt is made from.
	ent.mapEntity = mapEnt;

	// ensure we never resize, otherwise originals pointers fuck up.
	ent.brushes.reserve(mapEnt->GetNumPrimitives());

	// we process brushes / patches diffrent.
	for (i = 0; i < mapEnt->GetNumPrimitives(); i++)
	{
		prim = mapEnt->GetPrimitive(i);

		if (prim->getType() == PrimType::BRUSH)	{
			if (!processBrush(ent, static_cast<mapfile::XMapBrush*>(prim), i)) {
				X_ERROR("Lvl", "failed to process brush: %i", i);
				return false;
			}
		}
		else if (prim->getType() == PrimType::PATCH) {
			if (!processPatch(ent, static_cast<mapfile::XMapPatch*>(prim), i)) {
				X_ERROR("Lvl", "failed to process patch: %i", i);
				return false;
			}
		}
	}

	mapfile::XMapEntity::PairIt it = mapEnt->epairs.find(X_CONST_STRING("origin"));
	if (it != mapEnt->epairs.end())
	{
		// set the origin.
		const core::string& value = it->second;
		sscanf_s(value.c_str(), "%f %f %f",
			&ent.origin.x, &ent.origin.y, &ent.origin.z);
	}

	// check for angles.
	it = mapEnt->epairs.find(X_CONST_STRING("angles"));
	if (it != mapEnt->epairs.end())
	{
		const core::string& value = it->second;
		sscanf_s(value.c_str(), "%f %f %f",
			&ent.angle.x, &ent.angle.y, &ent.angle.z);
	}

	ent.bounds.clear();

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
		if (it != mapEnt->epairs.end())
		{ 
			core::string& name = it->second;
			// load the models bounding box.
			if (!ModelInfo::GetNModelAABB(name, ent.bounds))
			{
				X_ERROR("Lvl", "Failed to load model \"%s\" at (%g,%g,%g), using default",
				name.c_str(), ent.origin.x,ent.origin.y, ent.origin.z);
				it->second = "default";
				// give it the bounds of the default.
				// since I have no idea what the bounds of the missing model is \o/
				ent.bounds = defaultModelBounds_;
			}
		}
		else
		{
			X_ERROR("Lvl", "Ent with classname \"misc_model\" is missing \"model\" kvp at (%g,%g,%g)",
				ent.origin.x, ent.origin.y, ent.origin.z);
			return false;
		}
	}

	return true;
}

bool LvlBuilder::processBrush(LvlEntity& ent,
	mapfile::XMapBrush* mapBrush, int ent_idx)
{
	const mapfile::XMapBrushSide* pMapBrushSide;
	XWinding*	w;
	int32_t		i, numSides;


	LvlBrush& brush = ent.brushes.AddOne();
	brush.entityNum = stats_.numEntities;
	brush.brushNum = ent_idx;


	numSides = mapBrush->GetNumSides();
	for (i = 0; i < numSides; i++)
	{
		LvlBrushSide& side = brush.sides.AddOne();
		pMapBrushSide = mapBrush->GetSide(i);

		side.planenum = FindFloatPlane(pMapBrushSide->GetPlane());
		// material
		side.matInfo.name = pMapBrushSide->material.name;
		side.matInfo.matRepeate = pMapBrushSide->material.matRepeate;
		side.matInfo.rotate = pMapBrushSide->material.rotate;
		side.matInfo.shift = pMapBrushSide->material.shift;

		// load the material.
		side.matInfo.pMaterial = matMan_.loadMaterial(pMapBrushSide->material.name.c_str());
		if (!side.matInfo.pMaterial) {
			return false;
		}
	}

	if (!removeDuplicateBrushPlanes(brush)) {
		X_ERROR("Brush", "Failed to remove duplicate planes");
		return false;
	}

	if (!brush.calculateContents()) {
		X_ERROR("Brush", "Failed to calculate brush contents");
		return false;
	}

	// create windings for sides + bounds for brush
	if (!brush.createBrushWindings(planes)) {
		X_ERROR("Brush", "Failed to create windings for brush");
		return false;
	}

	// set original.
	brush.pOriginal = &brush;

	// check if we have a portal.
	if (brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL)) {
		stats_.numAreaPortals++;
	}


	Vec3f coords[2];
	Vec2f out;

	coords[0][0] = 1.0f;
	coords[1][1] = 1.0f;
	ConvertTexMatWithQTexture(coords, coords);

	Vec3f texX, texY;
	float u, v;

	for (i = 0; i < safe_static_cast<int32_t,size_t>(brush.sides.size()); i++)
	{
		LvlBrushSide& side = brush.sides[i];
		w = side.pWinding;

		if (!w) {
			continue;
		}

		pMapBrushSide = mapBrush->GetSide(i);
		Vec2f repeate = pMapBrushSide->material.matRepeate;
		Vec2f shift = pMapBrushSide->material.shift;

		ComputeAxisBase(pMapBrushSide->GetPlane().getNormal(), texX, texY);

		for (int j = 0; j < w->getNumPoints(); j++)
		{
			// gets me position from 0,0 from 2d plane.
			Vec5f& point = w->operator[](j);
			u = texX.dot(point.asVec3());
			v = texY.dot(point.asVec3());

			out[0] = coords[0][0] * u + coords[0][1] * v + coords[0][2];
			out[1] = coords[1][0] * u + coords[1][1] * v + coords[1][2];

			out += shift;

			// I have a repeate rate.
			out[0] = out[0] / repeate.x;
			out[1] = out[1] / repeate.y;

			point.s = out[0];
			point.t = -out[1];
		}
	}

	// stats.
	stats_.numBrushes++;

	return true;
}



bool LvlBuilder::processPatch(LvlEntity& ent, 
	mapfile::XMapPatch* mapPatch, int ent_idx)
{
	size_t i;

	if (gSettings.noPatches) { // are these goat meshes even allowed O_0 ?
		return false;
	}

	// meshes not supported yet.
//	if (mapBrush->isMesh()) {
//		return false;
//	}

	if (mapPatch->isMesh()) {
		mapPatch->SubdivideExplicit(mapPatch->GetHorzSubdivisions(),
			mapPatch->GetVertSubdivisions(), true);
	}
	else {
		mapPatch->Subdivide(DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true);
	}

	engine::IMaterial* pMaterial = matMan_.loadMaterial(mapPatch->GetMatName());

	// create a Primative
	for (i = 0; i < mapPatch->GetNumIndexes(); i += 3)
	{
		LvlTris& tri = ent.patches.AddOne();

		tri.pMaterial = pMaterial;
		tri.verts[2] = (*mapPatch)[mapPatch->GetIndexes()[i + 0]];
		tri.verts[1] = (*mapPatch)[mapPatch->GetIndexes()[i + 2]];
		tri.verts[0] = (*mapPatch)[mapPatch->GetIndexes()[i + 1]];
	}

	// stats
	stats_.numPatches++;
	return true;
}





// --------------------------------------------------------------


bool LvlBuilder::removeDuplicateBrushPlanes(LvlBrush& brush)
{
	size_t i, j;

	for (i = 1; i < brush.sides.size(); i++)
	{
		LvlBrushSide& side = brush.sides[i];

		// check for a degenerate plane
		if (side.planenum == -1)
		{
			X_WARNING("Brush", "Entity %i, Brush %i, Sides %i: "
				"degenerate plane(%i)", 
				brush.entityNum, brush.brushNum, brush.sides.size(), i);

			// remove it
			brush.sides.removeIndex(i);

			i--;
			continue;
		}

		// check for duplication and mirroring
		for (j = 0; j < i; j++) 
		{
			if (side.planenum == brush.sides[j].planenum)
			{
				X_WARNING("Brush", "Entity %i, Brush %i, Sides %i: "
					"duplicate plane(%i,%i)", 
					brush.entityNum, brush.brushNum, brush.sides.size(), i, j);

				// remove the second duplicate
				brush.sides.removeIndex(i);

				i--;
				break;
			}

			if (side.planenum == (brush.sides[i].planenum ^ 1))
			{
				// mirror plane, brush is invalid
				X_WARNING("Brush", "Entity %i, Brush %i, Sides %i: "
					"mirrored plane(%i,%i)", 
					brush.entityNum, brush.brushNum, brush.sides.size(), i, j);
				return false;
			}
		}
	}
	return true;
}
