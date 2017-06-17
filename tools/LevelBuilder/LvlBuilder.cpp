#include "stdafx.h"
#include "LvlBuilder.h"

#include "MapTypes.h"
#include "MapLoader.h"
#include "ModelCache.h"

namespace
{
	const Vec3f baseaxis[18] =
	{
		{ 1.0, 0.0, 0.0 },
		{ 0.0, -1.0, 0.0 },
		{ 0.0, 0.0, -1.0 },

		{ 1.0, 0.0, 0.0 },
		{ 0.0, -1.0, 0.0 },
		{ 1.0, 0.0, 0.0 },

		{ 0.0, 1.0, 0.0 },
		{ 0.0, 0.0, -1.0 },
		{ -1.0, 0.0, 0.0 },

		{ 0.0, 1.0, 0.0 },
		{ 0.0, 0.0, -1.0 },
		{ 0.0, 1.0, 0.0 },

		{ 1.0, 0.0, 0.0 },
		{ 0.0, 0.0, -1.0 },
		{ 0.0, -1.0, 0.0 },

		{ 1.0, 0.0, 0.0 },
		{ 0.0, 0.0, -1.0 },
		{ 0.0, -1.0, 0.0 }
	}; 


	X_DISABLE_WARNING(4244)
	void TextureAxisFromPlane(const Vec3f& normal, Vec3f& a2, Vec3f& a3)
	{
		size_t axis = 0;
		float largestAxis = 0.0;

		const float x = normal[0];
		const float y = normal[1];
		const float z = normal[2];

		const float negX = -normal[0];
		const float negY = -normal[1];
		const float negZ = -normal[2];

		// Z
		if (z > 0.0)
		{
			largestAxis = z;
			axis = 0; // positive Z
		}

		if (largestAxis < negZ)
		{
			largestAxis = negZ;
			axis = 1; // negative Z
		}

		// X
		if (largestAxis < x)
		{
			largestAxis = x;
			axis = 2; // positive X
		}

		if (largestAxis < negX)
		{
			largestAxis = negX;
			axis = 3; // negative Z
		}

		// Y
		if (largestAxis < y)
		{
			largestAxis = y;
			axis = 4; // positive Y
		}

		if (largestAxis < negY) {
			axis = 5; // negative Y
		}

		X_ASSERT(((3 * axis + 1) < 18),"axis out of range")(axis);
		a2 = baseaxis[3 * axis];
		a3 = baseaxis[3 * axis + 1];
	}

	X_ENABLE_WARNING(4244)


	static void QuakeTextureVecs(const Planef& plane, Vec2f shift, float rotate, Vec2f scale, Vec4f mappingVecs[2])
	{
		Vec3f	vecs[2];
		TextureAxisFromPlane(plane.getNormal(), vecs[0], vecs[1]);

		if (!scale[0]) {
			scale[0] = 1.f;
		}
		if (!scale[1]) {
			scale[1] = 1.f;
		}

		// rotate axis
		float ang, sinv, cosv;
		if (rotate == 0.f)
		{
			sinv = 0.f;
			cosv = 1.f;
		}
		else if (rotate == 90.f)
		{
			sinv = 1.f;
			cosv = 0.f;
		}
		else if (rotate == 180.f)
		{
			sinv = 0.f;
			cosv = -1.f;
		}
		else if (rotate == 270.f)
		{
			sinv = -1.f;
			cosv = 0.f;
		}
		else
		{
			ang = ::toRadians(rotate);
			sinv = sin(ang);
			cosv = cos(ang);
		}

		int32_t	sv, tv;
		if (vecs[0][0]) {
			sv = 0;
		}
		else if (vecs[0][1]) {
			sv = 1;
		}
		else {
			sv = 2;
		}

		if (vecs[1][0]) {
			tv = 0;
		}
		else if (vecs[1][1]) {
			tv = 1;
		}
		else {
			tv = 2;
		}

		for (int32_t i = 0; i < 2; i++) {
			float ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
			float nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
			vecs[i][sv] = ns;
			vecs[i][tv] = nt;
		}

		for (int32_t i = 0; i < 2; i++) {
			for (int32_t j = 0; j < 3; j++) {
				mappingVecs[i][j] = vecs[i][j] / scale[i];
			}
		}

		mappingVecs[0][3] = -(shift[0] / scale[0]);
		mappingVecs[1][3] = -(shift[1] / scale[1]);
	}

} // namespace




LvlBuilder::LvlBuilder(physics::IPhysicsCooking* pPhysCooking, core::MemoryArenaBase* arena) :
	arena_(arena),
	staticModels_(arena),
	entities_(arena),
	areas_(arena),

	multiRefEntLists_({ { arena, arena, arena, arena,
		arena, arena, arena, arena } }),

	multiModelRefLists_({ { arena, arena, arena, arena,
		arena, arena, arena, arena } }),

	stringTable_(arena),
	pMap_(nullptr),
	pPhysCooking_(pPhysCooking),
	matMan_(arena)
{
	core::zero_object(stats_);

	pModelCache_ = X_NEW(lvl::ModelCache, arena, "LvlModelCache")(arena);
}

LvlBuilder::~LvlBuilder()
{
	matMan_.ShutDown();

	if (pModelCache_) {
		X_DELETE(pModelCache_, arena_);
	}
}

bool LvlBuilder::init(void)
{
	if (!matMan_.Init()) {
		return false;
	}

	if (!pModelCache_->loadDefaultModel()) {
		return false;
	}

	return true;
}


bool LvlBuilder::LoadFromMap(mapfile::XMapFile* map)
{
	pMap_ = X_ASSERT_NOT_NULL(map);

	if (map->getNumEntities() == 0) {
		X_ERROR("Lvl", "Map has zero entites, atleast one is required");
		return false;
	}

	entities_.resize(map->getNumEntities());
	for (size_t i = 0; i < map->getNumEntities(); i++)
	{
		if (!processMapEntity(entities_[i], map->getEntity(i))) {
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
	X_LOG0("Map", "Total planes: ^8%" PRIuS, planes_.size());
	X_LOG0("Map", "Total areaPortals: ^8%" PRIi32, stats_.numAreaPortals);
	X_LOG0("Map", "Size: %s", mapBounds_.toString(boundsStr));
	return true;
}

int32_t LvlBuilder::FindFloatPlane(const Planef& plane)
{
	return planes_.FindPlane(plane, PLANE_NORMAL_EPSILON, PLANE_DIST_EPSILON);
}

bool LvlBuilder::processMapEntity(LvlEntity& ent, mapfile::XMapEntity* mapEnt)
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

		if (pPrim->getType() == PrimType::BRUSH)	{
			if (!processBrush(ent, static_cast<mapfile::XMapBrush*>(pPrim), i)) {
				X_ERROR("Lvl", "failed to process brush: %" PRIuS, i);
				return false;
			}
		}
		else if (pPrim->getType() == PrimType::PATCH) {
			if (!processPatch(ent, static_cast<mapfile::XMapPatch*>(pPrim), i)) {
				X_ERROR("Lvl", "failed to process patch: %" PRIuS, i);
				return false;
			}
		}
	}

	mapfile::XMapEntity::PairIt it = mapEnt->epairs.find(X_CONST_STRING("origin"));
	if (it != mapEnt->epairs.end())
	{
		// set the origin.
		const core::string& value = it->second;
		sscanf_s(value.c_str(), "%f %f %f", &ent.origin.x, &ent.origin.y, &ent.origin.z);
	}

	// check for angles.
	it = mapEnt->epairs.find(X_CONST_STRING("angles"));
	if (it != mapEnt->epairs.end())
	{
		const core::string& value = it->second;
		sscanf_s(value.c_str(), "%f %f %f", &ent.angle.x, &ent.angle.y, &ent.angle.z);
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
			const core::string& name = it->second;
			// load the models bounding box.
			if (!pModelCache_->getModelAABB(name, ent.bounds))
			{
				X_ERROR("Lvl", "Failed to load model \"%s\" at (%g,%g,%g), using default",
				name.c_str(), ent.origin.x,ent.origin.y, ent.origin.z);
				it->second = "default";
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
	mapfile::XMapBrush* mapBrush, size_t entIdx)
{
	LvlBrush& brush = ent.brushes.AddOne();
	brush.entityNum = stats_.numEntities;
	brush.brushNum = safe_static_cast<int32_t, size_t>(entIdx);

	size_t numSides = mapBrush->GetNumSides();
	for (size_t i = 0; i < numSides; i++)
	{
		LvlBrushSide& side = brush.sides.AddOne();
		auto* pMapBrushSide = mapBrush->GetSide(i);

		side.planenum = FindFloatPlane(pMapBrushSide->GetPlane());
		// material
		side.matInfo.name = pMapBrushSide->material.name;
		side.matInfo.matRepeate = pMapBrushSide->material.matRepeate;
		side.matInfo.rotate = pMapBrushSide->material.rotate;
		side.matInfo.shift = pMapBrushSide->material.shift;

		// load the material.
		side.matInfo.pMaterial = matMan_.loadMaterial(pMapBrushSide->material.name.c_str());
		if (!side.matInfo.pMaterial->isLoaded()) {
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
	if (!brush.createBrushWindings(planes_)) {
		X_ERROR("Brush", "Failed to create windings for brush");
		return false;
	}

	// set original.
	brush.pOriginal = &brush;

	// check if we have a portal.
	if (brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL)) {
		stats_.numAreaPortals++;
	}

	for (size_t i = 0; i < brush.sides.size(); i++)
	{
		const LvlBrushSide& side = brush.sides[i];
		auto* pWinding = side.pWinding;

		if (!pWinding) {
			continue;
		}
	
		auto* pMapBrushSide = mapBrush->GetSide(i);
		const Planef& plane = pMapBrushSide->GetPlane();
		const Vec2f& repeate = pMapBrushSide->material.matRepeate;
		const Vec2f& shift = pMapBrushSide->material.shift;
		const float& rotate = pMapBrushSide->material.rotate;

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

	// stats.
	stats_.numBrushes++;

	return true;
}



bool LvlBuilder::processPatch(LvlEntity& ent, 
	mapfile::XMapPatch* mapPatch, size_t entIdx)
{
	X_UNUSED(entIdx);

	if (gSettings.noPatches) { // are these goat meshes even allowed O_0 ?
		return false;
	}

	mapfile::XMapPatch& patch = *mapPatch;

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

	// stats
	stats_.numPatches++;
	return true;
}





// --------------------------------------------------------------


bool LvlBuilder::removeDuplicateBrushPlanes(LvlBrush& brush)
{
	for (size_t i = 1; i < brush.sides.size(); i++)
	{
		LvlBrushSide& side = brush.sides[i];

		// check for a degenerate plane
		if (side.planenum == -1)
		{
			X_WARNING("Brush", "Entity %" PRIi32 ", Brush %" PRIi32 ", Sides %" PRIuS ": degenerate plane(%" PRIuS ")",
				brush.entityNum, brush.brushNum, brush.sides.size(), i);

			// remove it
			brush.sides.removeIndex(i);

			i--;
			continue;
		}

		// check for duplication and mirroring
		for (size_t j = 0; j < i; j++) 
		{
			if (side.planenum == brush.sides[j].planenum)
			{
				X_WARNING("Brush", "Entity %" PRIi32 ", Brush %" PRIi32 ", Sides %" PRIuS ": duplicate plane(%" PRIuS ",%" PRIuS ")",
					brush.entityNum, brush.brushNum, brush.sides.size(), i, j);

				// remove the second duplicate
				brush.sides.removeIndex(i);

				i--;
				break;
			}

			if (side.planenum == (brush.sides[i].planenum ^ 1))
			{
				// mirror plane, brush is invalid
				X_WARNING("Brush", "Entity %" PRIi32 ", Brush %" PRIi32 ", Sides %" PRIuS ": mirrored plane(%" PRIuS ",%" PRIuS ")",
					brush.entityNum, brush.brushNum, brush.sides.size(), i, j);
				return false;
			}
		}
	}
	return true;
}
