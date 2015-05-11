#include "stdafx.h"

#include <String\Lexer.h>
#include <String\StackString.h>

#include <IFileSys.h>

#include "MapTypes.h"
#include "MapLoader.h"
#include "BSPTypes.h"

#include "TriTools.h"


X_USING_NAMESPACE;




// ---------


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