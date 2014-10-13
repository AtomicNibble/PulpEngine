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

BSPBuilder::BSPBuilder() :
entities(g_arena),
drawSurfs_(g_arena),
out_(g_arena)
{
	core::zero_object(stats_);

	drawSurfs_.reserve(4096 * 4);
}


bool BSPBuilder::LoadFromMap(mapfile::XMapFile* map)
{
	X_ASSERT_NOT_NULL(map);

	int i;
	int triSurfs;
	bspBrush* pBrush;
	bspTris* pTris;

	// Bitch i've told you once!
	// don't make me tell you again.
	if (map->getNumEntities() == 0) {
		X_WARNING("Bsp", "Map has zero entites, atleast one is required");
		return false;
	}
	
	entities.resize(map->getNumEntities());

	for (i = 0; i < map->getNumEntities(); i++) {
		processMapEntity(entities[i], map->getEntity(i));
	}

	triSurfs = 0;

	// get bounds of map.
	mapBounds.clear();
	for (pBrush = entities[0].pBrushes; pBrush; pBrush = pBrush->next) {
		mapBounds.add(pBrush->bounds);
	}
	for (pTris = entities[0].pPatches; pTris; pTris = pTris->next) {
		triSurfs++;
	}


	X_LOG0("Map", "Total world brush: %i", stats_.numBrushes);
	X_LOG0("Map", "Total world Surfs: %i", triSurfs);
	X_LOG0("Map", "Total patches: %i", stats_.numPatches);
	X_LOG0("Map", "Total entities: %i", stats_.numEntities);
	X_LOG0("Map", "Total planes: %i", this->planes.size());
	X_LOG0("Map", "Total areapotrals: %i", stats_.numAreaPortals);
	X_LOG0("Map", "Size: (%.0f,%.0f,%.0f) to (%.0f,%.0f,%.0f)", mapBounds.min[0], mapBounds.min[1], mapBounds.min[2],
		mapBounds.max[0], mapBounds.max[1], mapBounds.max[2]);

	return true;
}


int BSPBuilder::FindFloatPlane(const Planef& plane)
{
	return planes.FindPlane(plane, NORMAL_EPSILON, DIST_EPSILON);
}


bool BSPBuilder::processMapEntity(BspEntity& ent, mapfile::XMapEntity* mapEnt)
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



bool BSPBuilder::processBrush(BspEntity& ent, mapfile::XMapBrush* mapBrush, int ent_idx)
{
	const mapfile::XMapBrushSide* pMapBrushSide;
	BspSide*		pSide;
	bspBrush*		pBrush;
	int				i;

	stats_.numBrushes++;

	pBrush = AllocBrush(mapBrush->GetNumSides());

	pBrush->entityNum = stats_.numEntities;
	pBrush->brushNum = ent_idx;
	pBrush->numsides = mapBrush->GetNumSides();

	for (i = 0; i < mapBrush->GetNumSides(); i++)
	{
		pSide = &pBrush->sides[i];
		pMapBrushSide = mapBrush->GetSide(i);

		core::zero_this(pSide);

		pSide->planenum = FindFloatPlane(pMapBrushSide->GetPlane());
	}


	if (!removeDuplicateBrushPlanes(pBrush)) {
		FreeBrush(pBrush);
		return false;
	}


	// For now everything is opaque REKT
	pBrush->opaque = true;

	// create windings for sides + bounds for brush
	if (!pBrush->createBrushWindings(planes)) {
		FreeBrush(pBrush);
		return false;
	}


//	pBrush->original = Brush;

	// Add the primative.

	// Add to linked list.
	pBrush->next = ent.pBrushes;
	ent.pBrushes = pBrush;

	return true;
}



bool BSPBuilder::processPatch(BspEntity& ent, mapfile::XMapPatch* mapBrush, int ent_idx)
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


bool BSPBuilder::removeDuplicateBrushPlanes(bspBrush* pBrush)
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

bspBrush* BSPBuilder::AllocBrush(int numSides)
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

bspBrush* BSPBuilder::CopyBrush(bspBrush* pOth)
{
	X_ASSERT_NOT_NULL(pOth);

	const size_t baseSize = sizeof(bspBrush)-(6 * sizeof(BspSide));
	size_t requiredBytes = baseSize + (core::Max(6, pOth->numsides)*sizeof(BspSide));

	X_ASSERT(requiredBytes >= sizeof(bspBrush), "Size is invalid")(requiredBytes, sizeof(BspSide));

	bspBrush* pBrush = reinterpret_cast<bspBrush*>(X_NEW_ARRAY(uint8_t, requiredBytes, g_arena, "bspBrush"));

	core::Mem::Construct<bspBrush>(pBrush, *pOth);


	return pBrush;
}

void BSPBuilder::FreeBrush(bspBrush* pBrush)
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

bspFace* BSPBuilder::AllocBspFace(void)
{
	return X_NEW(bspFace,g_arena,"bspFace");
}

void BSPBuilder::FreeBspFace(bspFace* pFace)
{
	X_DELETE(pFace,g_arena);
}


bspTree* BSPBuilder::AllocTree(void)
{
	return X_NEW(bspTree, g_arena, "bspTree");

}

void BSPBuilder::FreeTree(bspTree* pTree)
{
	X_DELETE(pTree, g_arena);
}


bspNode* BSPBuilder::AllocNode(void)
{
	return X_NEW(bspNode, g_arena, "bspNode");
}

void BSPBuilder::FreeNode(bspNode* pNode)
{
	X_DELETE(pNode, g_arena);
}

bspDrawSurface* BSPBuilder::AllocDrawSurface(DrawSurfaceType::Enum type)
{
	bspDrawSurface* pDs;

	drawSurfs_.append(bspDrawSurface());

	pDs = drawSurfs_.end() - 1;
	pDs->type = type;
	pDs->planeNum = -1;
	pDs->outputNum = -1;                    
	pDs->surfaceNum = safe_static_cast<int,size_t>(drawSurfs_.size() - 1);  

	return pDs;
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