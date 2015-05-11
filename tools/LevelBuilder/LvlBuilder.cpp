#include "stdafx.h"
#include "LevelBuilder.h"

#include "MapTypes.h"
#include "MapLoader.h"


namespace
{
	static const float NORMAL_EPSILON = 0.00001f;
	static const float DIST_EPSILON = 0.01f;

	const float DEFAULT_CURVE_MAX_ERROR = 4.0f;
	const float DEFAULT_CURVE_MAX_LENGTH = -1.0f;


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
entities_(g_arena),
areas_(g_arena),
stringTable_(g_arena)
{
	core::zero_object(stats_);
}


bool LvlBuilder::LoadFromMap(mapfile::XMapFile* map)
{
	X_ASSERT_NOT_NULL(map);
	int32_t i;

	if (map->getNumEntities() == 0) {
		X_WARNING("Bsp", "Map has zero entites, atleast one is required");
		return false;
	}

	entities_.resize(map->getNumEntities());
	for (i = 0; i < map->getNumEntities(); i++)
	{
		processMapEntity(entities_[i], map->getEntity(i));
	}


	X_LOG0("Map", "Total world brush: %i", entities_[0].brushes.size());
	X_LOG0("Map", "Total world patches: %i", 0); // TODO
	X_LOG0("Map", "Total total brush: %i", stats_.numBrushes);
	X_LOG0("Map", "Total total patches: %i", stats_.numPatches);
	X_LOG0("Map", "Total entities: %i", stats_.numEntities);
	X_LOG0("Map", "Total planes: %i", this->planes.size());
	X_LOG0("Map", "Total areapotrals: %i", stats_.numAreaPortals);
	X_LOG0("Map", "Size: (%.0f,%.0f,%.0f) to (%.0f,%.0f,%.0f)", 
		mapBounds.min[0], mapBounds.min[1], mapBounds.min[2],
		mapBounds.max[0], mapBounds.max[1], mapBounds.max[2]);

	return true;
}

int32_t LvlBuilder::FindFloatPlane(const Planef& plane)
{
	return planes.FindPlane(plane, NORMAL_EPSILON, DIST_EPSILON);
}

bool LvlBuilder::processMapEntity(LvlEntity& ent, mapfile::XMapEntity* mapEnt)
{
	mapfile::XMapPrimitive* prim;
	int32_t i;

	// update stats.
	stats_.numEntities++;

	// the map ent this LvlEnt is made from.
	ent.mapEntity = mapEnt;

	// we process brushes / patches diffrent.
	for (i = 0; i < mapEnt->GetNumPrimitives(); i++)
	{
		prim = mapEnt->GetPrimitive(i);

		if (prim->getType() == PrimType::BRUSH)	{
			processBrush(ent, static_cast<mapfile::XMapBrush*>(prim), i);
		}
		else if (prim->getType() == PrimType::PATCH) {
			processPatch(ent, static_cast<mapfile::XMapPatch*>(prim), i);
		}
	}

	mapfile::XMapEntity::PairIt it = mapEnt->epairs.find("origin");
	if (it != mapEnt->epairs.end())
	{
		// set the origin.
		const core::string& value = it->second;
		sscanf(value.c_str(), "%f %f %f",
			&ent.origin.x, &ent.origin.y, &ent.origin.z);
	}

	return true;
}

bool LvlBuilder::processBrush(LvlEntity& ent,
	mapfile::XMapBrush* mapBrush, int ent_idx)
{
	const mapfile::XMapBrushSide* pMapBrushSide;
	XWinding*	w;
	int32_t		i, numSides;


	LvlBrush brush;
	brush.entityNum = stats_.numEntities;
	brush.brushNum = ent_idx;

	core::StackString<level::MAP_MAX_MATERIAL_LEN> lastMatName;

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

		// see if mat names the same.
		if (i == 0)
		{
			lastMatName = pMapBrushSide->material.name;
		}
		else
		{
			if (lastMatName != pMapBrushSide->material.name) 
			{
				brush.allsidesSameMat = false;
			}
		}
	}

	if (!removeDuplicateBrushPlanes(brush))
	{
		return false;
	}

	// create windings for sides + bounds for brush
	if (!brush.createBrushWindings(planes)) {
		return false;
	}


	Vec3f coords[2];
	Vec2f out;
	Vec2f size(128, 128);

	coords[0][0] = 1.0f;
	coords[1][1] = 1.0f;
	ConvertTexMatWithQTexture(coords, coords);

	Vec3f texX, texY;
	float u, v;

	for (i = 0; i < numSides; i++)
	{
		LvlBrushSide& side = brush.sides[i];
		w = side.pWinding;

		if (!w) {
			continue;
		}

		pMapBrushSide = mapBrush->GetSide(i);
		Vec2f repeate = pMapBrushSide->material.matRepeate;

		ComputeAxisBase(pMapBrushSide->GetPlane().getNormal(), texX, texY);

		for (int j = 0; j < w->GetNumPoints(); j++)
		{
			// gets me position from 0,0 from 2d plane.
			Vec5f& point = w->operator[](j);
			u = texX.dot(point.asVec3());
			v = texY.dot(point.asVec3());

			out[0] = coords[0][0] * u + coords[0][1] * v + coords[0][2];
			out[1] = coords[1][0] * u + coords[1][1] * v + coords[1][2];

			// I have a repeate rate.
			out[0] = out[0] / size.x;
			out[1] = out[1] / size.y;

			point.s = out[0];
			point.t = out[1];
		}
	}

	// stats.
	stats_.numBrushes++;
	// add to end.
	ent.brushes.append(brush);

	return true;
}



bool LvlBuilder::processPatch(LvlEntity& ent, 
	mapfile::XMapPatch* mapBrush, int ent_idx)
{
#if 0
	bspTris*		pTri;
	int				i;


	if (gSettings.noPatches) { // are these goat meshes even allowed O_0 ?
		return false;
	}

	// meshes not supported yet.
	if (mapBrush->isMesh()) {
		return false;
	}

	stats_.numPatches++;
	ent.numPatches++;

	mapBrush->Subdivide(DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_ERROR, DEFAULT_CURVE_MAX_LENGTH, true);

	// create a Primative

	// Add the primative.
	pTri = X_NEW_ARRAY(bspTris, mapBrush->GetNumIndexes(), g_arena, "PatchTris");

	// Add to linked list.
	pTri->next = ent.pPatches;
	ent.pPatches = pTri;


	for (i = 0; i < mapBrush->GetNumIndexes(); i += 3)
	{
		pTri->v[2] = (*mapBrush)[mapBrush->GetIndexes()[i + 0]];
		pTri->v[1] = (*mapBrush)[mapBrush->GetIndexes()[i + 2]];
		pTri->v[0] = (*mapBrush)[mapBrush->GetIndexes()[i + 1]];
		//	tri->material = material;
		//	tri->next = prim->tris;
		//	prim->tris = tri;
	}
#endif
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
			if (side.planenum == brush.sides[i].planenum)
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
