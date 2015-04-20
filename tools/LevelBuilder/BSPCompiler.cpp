#include "stdafx.h"

#include <String\Lexer.h>
#include <String\StackString.h>

#include <IFileSys.h>

#include "MapTypes.h"
#include "MapLoader.h"
#include "BSPTypes.h"

#include "TriTools.h"

// MapGlobals gMapGlobals;
Settings gSettings;

namespace
{
	static const float NORMAL_EPSILON = 0.00001f;
	static const float DIST_EPSILON = 0.01f;

	const float DEFAULT_CURVE_MAX_ERROR = 4.0f;
	const float DEFAULT_CURVE_MAX_LENGTH = -1.0f;
}

X_USING_NAMESPACE;

LvlBuilder::LvlBuilder() :
entities_(g_arena),
areas_(g_arena),

stringTable_(g_arena),

data_(g_arena)

{
	core::zero_object(stats_);

}


bool LvlBuilder::LoadFromMap(mapfile::XMapFile* map)
{
	X_ASSERT_NOT_NULL(map);

	int i;
	bspBrush* pBrush;

	// Bitch i've told you once!
	// don't make me tell you again.
	if (map->getNumEntities() == 0) {
		X_WARNING("Bsp", "Map has zero entites, atleast one is required");
		return false;
	}
	
	entities_.resize(map->getNumEntities());

	for (i = 0; i < map->getNumEntities(); i++)
	{
		processMapEntity(entities_[i], map->getEntity(i));
	}


	// get bounds of map.
	mapBounds.clear();


	for (pBrush = entities_[0].pBrushes; pBrush; pBrush = pBrush->next) {
		mapBounds.add(pBrush->bounds);
	}

	// TODO: add patches to bounds?


	X_LOG0("Map", "Total world brush: %i", entities_[0].numBrushes);
	X_LOG0("Map", "Total world patches: %i", entities_[0].numBrushes);
	X_LOG0("Map", "Total total brush: %i", stats_.numBrushes);
	X_LOG0("Map", "Total total patches: %i", stats_.numPatches);
	X_LOG0("Map", "Total entities: %i", stats_.numEntities);
	X_LOG0("Map", "Total planes: %i", this->planes.size());
	X_LOG0("Map", "Total areapotrals: %i", stats_.numAreaPortals);
	X_LOG0("Map", "Size: (%.0f,%.0f,%.0f) to (%.0f,%.0f,%.0f)", mapBounds.min[0], mapBounds.min[1], mapBounds.min[2],
		mapBounds.max[0], mapBounds.max[1], mapBounds.max[2]);

	return true;
}


int LvlBuilder::FindFloatPlane(const Planef& plane)
{
	return planes.FindPlane(plane, NORMAL_EPSILON, DIST_EPSILON);
}


bool LvlBuilder::processMapEntity(LvlEntity& ent, mapfile::XMapEntity* mapEnt)
{
	mapfile::XMapPrimitive* prim;
	int i;
	core::zero_object(ent);
	ent.mapEntity = mapEnt;

	stats_.numEntities++;

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
		sscanf(value.c_str(), "%f %f %f", &ent.origin.x, &ent.origin.y, &ent.origin.z);
	}

	return true;
}

void ComputeAxisBase(Vec3f normal, Vec3f& texS, Vec3f& texT)
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

void ConvertTexMatWithQTexture(Vec3f texMat1[2], Vec3f texMat2[2])
{
	float s1, s2;
	s1 = ( 512.0f) / ( 512.0f);
	s2 = (512.0f) / (512.0f);
	texMat2[0][0] = s1 * texMat1[0][0];
	texMat2[0][1] = s1 * texMat1[0][1];
	texMat2[0][2] = s1 * texMat1[0][2];
	texMat2[1][0] = s2 * texMat1[1][0];
	texMat2[1][1] = s2 * texMat1[1][1];
	texMat2[1][2] = s2 * texMat1[1][2];
}

bool LvlBuilder::processBrush(LvlEntity& ent, mapfile::XMapBrush* mapBrush, int ent_idx)
{
	const mapfile::XMapBrushSide* pMapBrushSide;
	BspSide*		pSide;
	bspBrush*		pBrush;
	XWinding*	w;
	int				i, numSides;

	stats_.numBrushes++;
	ent.numBrushes++;

	pBrush = AllocBrush(mapBrush->GetNumSides());

	pBrush->entityNum = stats_.numEntities;
	pBrush->brushNum = ent_idx;
	pBrush->numsides = mapBrush->GetNumSides();
	pBrush->allsidesSameMat = true;


	core::StackString<bsp::MAP_MAX_MATERIAL_LEN> lastMatName;

	numSides = mapBrush->GetNumSides();
	for (i = 0; i < numSides; i++)
	{
		pSide = &pBrush->sides[i];
		pMapBrushSide = mapBrush->GetSide(i);

		core::zero_this(pSide);

		pSide->planenum = FindFloatPlane(pMapBrushSide->GetPlane());
		// material
		pSide->material.name = pMapBrushSide->material.name;
		pSide->material.matRepeate = pMapBrushSide->material.matRepeate;
		pSide->material.rotate = pMapBrushSide->material.rotate;
		pSide->material.shift = pMapBrushSide->material.shift;

		if (i == 0) {
			lastMatName = pMapBrushSide->material.name;
		}
		else {
			if (lastMatName != pMapBrushSide->material.name) {
				pBrush->allsidesSameMat = false;
			}
		}
	}


	if (!removeDuplicateBrushPlanes(pBrush)) {
		FreeBrush(pBrush);
		ent.numBrushes--;
		return false;
	}


	// For now everything is opaque REKT
	pBrush->opaque = true;

	// create windings for sides + bounds for brush
	if (!pBrush->createBrushWindings(planes)) {
		FreeBrush(pBrush);
		ent.numBrushes--;
		return false;
	}


	Vec3f coords[2];
	Vec2f out;
	Vec2f size(512, 512);

	coords[0][0] = 1.0f;
	coords[1][1] = 1.0f;
	ConvertTexMatWithQTexture(coords, coords);

	Vec3f texX, texY;
	float u, v;

	for (i = 0; i < numSides; i++)
	{

		pSide = &pBrush->sides[i];
		w = pSide->pWinding;
		pMapBrushSide = mapBrush->GetSide(i);

		Vec2f repeate = pMapBrushSide->material.matRepeate;

		if (!w)
			continue;

		ComputeAxisBase(pMapBrushSide->GetPlane().getNormal(),
			texX, texY);

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

	// Add to linked list.
	pBrush->next = ent.pBrushes;
	ent.pBrushes = pBrush;

	return true;
}



bool LvlBuilder::processPatch(LvlEntity& ent, mapfile::XMapPatch* mapBrush, int ent_idx)
{
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

	return true;
}


// --------------------------------------------------------------


bool LvlBuilder::removeDuplicateBrushPlanes(bspBrush* pBrush)
{
	int			i, j, k;
	BspSide*		pSides;
	bspBrush*		b;

	pSides = pBrush->sides;
	b = pBrush;

	for (i = 1; i < b->numsides; i++) 
	{
		// check for a degenerate plane
		if (pSides[i].planenum == -1) {
			X_WARNING("Brush", "Entity %i, Brush %i, Sides %i: degenerate plane", b->entityNum, b->brushNum, b->numsides);
			// remove it
			for (k = i + 1; k < b->numsides; k++) {
				pSides[k - 1] = pSides[k];
			}
			b->numsides--;
			i--;
			continue;
		}

		// check for duplication and mirroring
		for (j = 0; j < i; j++) {
			if (pSides[i].planenum == pSides[j].planenum) {
				X_WARNING("Brush", "Entity %i, Brush %i, Sides %i: duplicate plane", b->entityNum, b->brushNum, b->numsides);
				// remove the second duplicate
				for (k = i + 1; k < b->numsides; k++) {
					pSides[k - 1] = pSides[k];
				}
				b->numsides--;
				i--;
				break;
			}

			if (pSides[i].planenum == (pSides[j].planenum ^ 1)) {
				// mirror plane, brush is invalid
				X_WARNING("Brush", "Entity %i, Brush %i, Sides %i: mirrored plane", b->entityNum, b->brushNum, b->numsides);
				return false;
			}
		}
	}
	return true;
}


// ---------------------- Alocators ------------------------

bspBrush* LvlBuilder::AllocBrush(int numSides)
{
	// allows for allocating objects which allow overrun.
	const size_t baseSize = sizeof(bspBrush)-(6 * sizeof(BspSide));
	size_t requiredBytes = baseSize + (core::Max(6, numSides)*sizeof(BspSide));

	X_ASSERT(requiredBytes >= sizeof(bspBrush), "Size is invalid")(requiredBytes, sizeof(BspSide));

	bspBrush* pBrush = reinterpret_cast<bspBrush*>(X_NEW_ARRAY(uint8_t, requiredBytes, g_arena, "bspBrush"));

	core::Mem::Construct<bspBrush>(pBrush);
	if (numSides > 6)
	{
		// contruct any BspSides that are missed in object contrustor.
		int i;
		for (i = 6; i < numSides; i++)  {
			core::Mem::Construct<BspSide>(&pBrush->sides[i]);
		}
	}

	return pBrush;
}

bspBrush* LvlBuilder::CopyBrush(bspBrush* pOth)
{
	X_ASSERT_NOT_NULL(pOth);

	const size_t baseSize = sizeof(bspBrush)-(6 * sizeof(BspSide));
	size_t requiredBytes = baseSize + (core::Max(6, pOth->numsides)*sizeof(BspSide));

	X_ASSERT(requiredBytes >= sizeof(bspBrush), "Size is invalid")(requiredBytes, sizeof(BspSide));

	bspBrush* pBrush = reinterpret_cast<bspBrush*>(X_NEW_ARRAY(uint8_t, requiredBytes, g_arena, "bspBrush"));

	core::Mem::Construct<bspBrush>(pBrush, *pOth);


	return pBrush;
}

void LvlBuilder::FreeBrush(bspBrush* pBrush)
{
	X_ASSERT_NOT_NULL(pBrush);
	uint8_t* pData = reinterpret_cast<uint8_t*>(pBrush);

	// Destruct everything like a good potato.
	if (pBrush->numsides > 6)
	{
		int i;
		for (i = 6; i < pBrush->numsides; i++)  {
			core::Mem::Construct<BspSide>(&pBrush->sides[i]);
		}
	}

	core::Mem::Destruct<bspBrush>(pBrush);

	X_DELETE_ARRAY(pData, g_arena);
}

bspFace* LvlBuilder::AllocBspFace(void)
{
	return X_NEW(bspFace,g_arena,"bspFace");
}

void LvlBuilder::FreeBspFace(bspFace* pFace)
{
	X_DELETE(pFace,g_arena);
}


bspTree* LvlBuilder::AllocTree(void)
{
	return X_NEW(bspTree, g_arena, "bspTree");

}

void LvlBuilder::FreeTree(bspTree* pTree)
{
	X_DELETE(pTree, g_arena);
}


bspNode* LvlBuilder::AllocNode(void)
{
	return X_NEW(bspNode, g_arena, "bspNode");
}

void LvlBuilder::FreeNode(bspNode* pNode)
{
	X_DELETE(pNode, g_arena);
}


/*
void BSPData::WriteBspMap(primitive_t *list)
{
	uBrush_t	*b;
	int i, x;
	XWinding *	w;
	side_t *	s;


	core::XFileScoped file;
	core::fileModeFlags mode;
	mode.Set(core::fileMode::WRITE);
	mode.Set(core::fileMode::RECREATE);

	x = 0;

	if (file.openFile("C:\\Users\\Tom\\Documents\\Visual Studio 2013\\Projects\\WinEngine\\code\\game_folder\\dump_bsp.map", mode))
	{
		file.printf("iwmap 4\n");
		file.printf("\"000_Global\" flags expanded  active\n");
		file.printf("{\n\"classname\" \"worldspawn\"\n");
		file.printf("// entity 0\n");

		for (; list; list = list->next)
		{
			b = list->brush;
			if (!b) {
				continue;
			}

			file.printf("// brush 1\n{\n", x++);

			for (i = 0, s = b->sides; i < b->numsides; i++, s++)
			{
				w = new XWinding(planes[s->planenum]);

				file.printf("	( %i %i %i ) ", (int)(*w)[0][0], (int)(*w)[0][1], (int)(*w)[0][2]);
				file.printf("( %i %i %i ) ", (int)(*w)[1][0], (int)(*w)[1][1], (int)(*w)[1][2]);
				file.printf("( %i %i %i ) ", (int)(*w)[2][0], (int)(*w)[2][1], (int)(*w)[2][2]);

				file.printf("caulk 64 64 0 0 0 0 ");
				file.printf("notexture 64 64 0 0 0 0\n");
				delete w;
			}
			file.printf("}\n");
		}
		file.printf("}\n");
	}
}
*/


/*

bool BSPData::ProcessModel(uEntity_t *e, bool floodFill)
{
	bspface_t	*faces;

	faces = MakeStructuralBspFaceList(e->primitives);
	e->tree = FaceBSP(faces);

	// just make structualy info now, cus i'm lazy
#if 1

//	if (floodFill)
//		WriteBspMap(e->primitives);

	// create portals at every leaf intersection
	// to allow flood filling
	MakeTreePortals(e->tree);

	FilterBrushesIntoTree(e);

	if (FloodEntities(e->tree))
	{
		// set the outside leafs to opaque
		FillOutside(e);
	}
	else 
	{
		X_ERROR("BSP", "******* leaked *******");
	//	LeakFile(e->tree);
		return false;
	}


	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ClipSidesByTree(e);

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	FloodAreas(e);

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas(e);

	// now build shadow volumes for the lights and split
	// the optimize lists by the light beam trees
	// so there won't be unneeded overdraw in the static
	// case
	Prelight(e);


	// optimizing is a superset of fixing tjunctions
	if (!gSettings.noOptimize) {
//		OptimizeEntity(e);
	}
	else  if (!gSettings.noTJunc) {
		FixEntityTjunctions(e);
	}

	// now fix t junctions across areas
	FixGlobalTjunctions(e);
#endif

	return true;
}


bool BSPData::ProcessModels()
{
	int entityNum;
	uEntity_t	*entity;

	for (entityNum = 0; entityNum < numEntities; entityNum++) 
	{
		entity = &entities[entityNum];
		if (!entity->primitives) {
			continue;
		}

		X_LOG0("Entity", "----------- entity %i -----------", entityNum);

		// if we leaked, stop without any more processing
		if (!ProcessModel(entity, (bool)(entityNum == 0))) {
			return false;
		}
	}

	return true;
}

*/