#include "stdafx.h"
#include "LvlBuilder.h"

#include "MapTypes.h"
#include "MapLoader.h"
#include "ModelInfo.h"


namespace
{
	Vec3f baseaxis[18] =
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
		size_t axis; 
		float largestAxis;

		axis = 0;
		largestAxis = 0.0;

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
		int		sv, tv;
		float	ang, sinv, cosv;
		float	ns, nt;
		int		i, j;

		TextureAxisFromPlane(plane.getNormal(), vecs[0], vecs[1]);

		if (!scale[0]) {
			scale[0] = 1.f;
		}
		if (!scale[1]) {
			scale[1] = 1.f;
		}

		// rotate axis
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

		for (i = 0; i < 2; i++) {
			ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
			nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
			vecs[i][sv] = ns;
			vecs[i][tv] = nt;
		}

		for (i = 0; i < 2; i++) {
			for (j = 0; j < 3; j++) {
				mappingVecs[i][j] = vecs[i][j] / scale[i];
			}
		}

		mappingVecs[0][3] = -(shift[0] / scale[0]);
		mappingVecs[1][3] = -(shift[1] / scale[1]);
	}

} // namespace




LvlBuilder::LvlBuilder() :
	staticModels_(g_arena),
	entities_(g_arena),
	areas_(g_arena),

	multiRefEntLists_({ { g_arena, g_arena, g_arena, g_arena,
	g_arena, g_arena, g_arena, g_arena } }),

	multiModelRefLists_({ { g_arena, g_arena, g_arena, g_arena,
		g_arena, g_arena, g_arena, g_arena } }),

	stringTable_(g_arena),
	pMap_(nullptr),
	matMan_(g_arena)
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
	size_t i;

	pMap_ = map;

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
	X_LOG0("Map", "Total brush: ^8%i", stats_.numBrushes);
	X_LOG0("Map", "Total patches: ^8%i", stats_.numPatches);
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
	size_t i;

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
	mapfile::XMapBrush* mapBrush, size_t entIdx)
{
	const mapfile::XMapBrushSide* pMapBrushSide;
	XWinding*	w;
	size_t		i, numSides;


	LvlBrush& brush = ent.brushes.AddOne();
	brush.entityNum = stats_.numEntities;
	brush.brushNum = safe_static_cast<int32_t, size_t>(entIdx);


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

	for (i = 0; i < brush.sides.size(); i++)
	{
		const LvlBrushSide& side = brush.sides[i];
		w = side.pWinding;

		if (!w) {
			continue;
		}
	
		pMapBrushSide = mapBrush->GetSide(i);
		const Planef& plane = pMapBrushSide->GetPlane();
		const Vec2f& repeate = pMapBrushSide->material.matRepeate;
		const Vec2f& shift = pMapBrushSide->material.shift;
		const float& rotate = pMapBrushSide->material.rotate;

		Vec4f mappingVecs[2];
		QuakeTextureVecs(plane, shift, rotate, repeate, mappingVecs);

		for (size_t j = 0; j < w->getNumPoints(); j++)
		{
			// gets me position from 0,0 from 2d plane.
			Vec5f& point = w->operator[](j);
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
	size_t i;

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
	for (i = 0; i < patch.GetNumIndexes(); i += 3)
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
