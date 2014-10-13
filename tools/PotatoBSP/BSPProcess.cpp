#include "stdafx.h"
#include "BSPTypes.h"



bool BSPBuilder::ProcessModels(void)
{
	int entityNum;
	int numEntities = stats_.numEntities;

	for (entityNum = 0; entityNum < numEntities; entityNum++)
	{
		const BspEntity& entity = entities[entityNum];
		if (!entity.pBrushes && !entity.pPatches) {
			continue;
		}

		X_LOG0("Entity", "----------- entity %i -----------", entityNum);

		// if we leaked, stop without any more processing
		if (entityNum == 0)
		{
			if (!ProcessWorldModel(entity))
				return false;
		}
		else
		{
			if (!ProcessModel(entity)) 
				return false;
		}
	}

	return true;
}



bool BSPBuilder::ProcessModel(const BspEntity& ent)
{
	


	return true;
}

bool BSPBuilder::ProcessWorldModel(const BspEntity& ent)
{
	bspFace*	pFaces;
	bspTree*	pTree;

	pFaces = MakeStructuralBspFaceList(ent.pBrushes);
	pTree = FaceBSP(pFaces);


	FilterStructuralBrushesIntoTree(ent, pTree);


	ClipSidesIntoTree(ent, pTree);


	AddEntitySurfaceModels(ent);



	// ydnar: meta surfaces 
	MakeEntityMetaTriangles(ent);
//	SmoothMetaTriangles();
//	FixMetaTJunctions();
//	MergeMetaTriangles();


	FilterDrawsurfsIntoTree(ent, pTree);


	return true;
}



// ---------------------------------------------



void BSPBuilder::AddEntitySurfaceModels(const BspEntity& ent)
{
	int i;
	int num = safe_static_cast<int, size_t>(drawSurfs_.size());

	int numSurfaceModels = 0;
	for (i = ent.firstDrawSurf; i < num; i++)
	{
		numSurfaceModels += AddSurfaceModels(drawSurfs_[i]);
	}

}

int BSPBuilder::AddSurfaceModels(const bspDrawSurface& surface)
{

	return 1;
}


// ---------------------------------------------

/*
FilterDrawsurfsIntoTree()
upon completion, all drawsurfs that actually generate a reference
will have been emited to the bspfile arrays, and the references
will have valid final indexes
*/

void BSPBuilder::FilterDrawsurfsIntoTree(const BspEntity& ent, bspTree* pTree)
{
	int i;
	bspDrawSurface    *ds;

	Vec3f origin;
	// mins, maxs;

	int refs;
	int numSurfs, numRefs, numSkyboxSurfaces;


	X_LOG0("Bsp", "--- FilterDrawsurfsIntoTree ---");

	numSurfs = 0;
	numRefs = 0;
	numSkyboxSurfaces = 0;
	for (i = ent.firstDrawSurf; i < (int)drawSurfs_.size(); i++)
	{
		/* get surface and try to early out */
		ds = &drawSurfs_[i];
		if (ds->numVerts == 0) {
			continue;
		}

		refs = 0;

		switch (ds->type)
		{
			case DrawSurfaceType::FACE:
				if (refs == 0) 
					refs = FilterFaceIntoTree(ds, pTree);
				if (refs > 0) 
					EmitFaceSurface(ds);
			break;

			default:
				refs = 0;
			break;
		}

		// tot up the references 
		if (refs > 0)
		{
			// tot up counts 
			numSurfs++;
			numRefs += refs;

			// emit extra surface data 
		//	SetSurfaceExtra(ds, numBSPDrawSurfaces - 1);
			//%	Sys_FPrintf( SYS_VRB, "%d verts %d indexes\n", ds->numVerts, ds->numIndexes );

			// one last sanity check 
#if 0
			{
				bspDrawSurface_t    *out;
				out = &bspDrawSurfaces[numBSPDrawSurfaces - 1];
				if (out->numVerts == 3 && out->numIndexes > 3) {
					Sys_Printf("\nWARNING: Potentially bad %s surface (%d: %d, %d)\n     %s\n",
						surfaceTypes[ds->type],
						numBSPDrawSurfaces - 1, out->numVerts, out->numIndexes, si->shader);
				}
			}
#endif
		}
	}

	/* emit some statistics */
	X_LOG0("Stats", "%9d references", numRefs);
/*	X_LOG0("Stats", "%9d (%d) emitted drawsurfs", numSurfs, numBSPDrawSurfaces);
	X_LOG0("Stats", "%9d stripped face surfaces", numStripSurfaces);
	X_LOG0("Stats", "%9d fanned face surfaces", numFanSurfaces);
	X_LOG0("Stats", "%9d surface models generated", numSurfaceModels);
	X_LOG0("Stats", "%9d skybox surfaces generated", numSkyboxSurfaces);
	for (i = 0; i < DrawSurfaceType::ENUM_COUNT; i++)
		X_LOG0("Stats", "%9d %s surfaces", numSurfacesByType[i], surfaceTypes[i]);

	X_LOG0("Stats", "%9d redundant indexes supressed, saving %d Kbytes", numRedundantIndexes, (numRedundantIndexes * 4 / 1024));
	*/
}

int BSPBuilder::FilterFaceIntoTree(bspDrawSurface* ds, bspTree* pTree)
{
//	XWinding* w;
//	int refs = 0;

	// make a winding and filter it into the tree 
//	w = WindingFromDrawSurf(ds);
//	refs = FilterWindingIntoTree_r(w, ds, pTree->headnode);


	return 1;
}



void BSPBuilder::EmitFaceSurface(bspDrawSurface* ds)
{
	EmitTriangleSurface(ds);
}


/*
EmitTriangleSurface()
creates a bsp drawsurface from arbitrary triangle surfaces
*/

void BSPBuilder::EmitTriangleSurface(bspDrawSurface* ds)
{
//	int i, temp;
	bsp::Surface        *out;


	// invert the surface if necessary 
/*	if (ds->backSide || ds->shaderInfo->invert)
	{
		// walk the indexes, reverse the triangle order 
		for (i = 0; i < ds->numIndexes; i += 3)
		{
			temp = ds->indexes[i];
			ds->indexes[i] = ds->indexes[i + 1];
			ds->indexes[i + 1] = temp;
		}

		// walk the verts, flip the normal 
		for (i = 0; i < ds->numVerts; i++)
			VectorScale(ds->verts[i].normal, -1.0f, ds->verts[i].normal);

		// invert facing 
		VectorScale(ds->lightmapVecs[2], -1.0f, ds->lightmapVecs[2]);
	}
	*/

	// allocate a new surface 
//	if (numBSPDrawSurfaces == MAX_MAP_DRAW_SURFS) {
//		Error("MAX_MAP_DRAW_SURFS");
//	}
//	out = &bspDrawSurfaces[numBSPDrawSurfaces];
//	ds->outputNum = numBSPDrawSurfaces;
//	numBSPDrawSurfaces++;
//	memset(out, 0, sizeof(*out));


	out_.surfaces.append(bsp::Surface());
	out = out_.surfaces.end() - 1;
	out->materialIdx = -1;
	out->surfaceType = bsp::SurfaceType::Patch;

	ds->outputNum = safe_static_cast<int,size_t>(out_.surfaces.size());



	/* ydnar: gs mods: handle lightmapped terrain (force to planar type) */
	//%	else if( VectorLength( ds->lightmapAxis ) <= 0.0f || ds->type == SURFACE_TRIANGLES || ds->type == SURFACE_FOGHULL || debugSurfaces )
/*	else if ((VectorLength(ds->lightmapAxis) <= 0.0f && ds->planar == qfalse) ||
		ds->type == SURFACE_TRIANGLES ||
		ds->type == SURFACE_FOGHULL ||
		ds->numVerts > maxLMSurfaceVerts ||
		debugSurfaces) {
		out->surfaceType = MST_TRIANGLE_SOUP;
	}
	else */
	{
//		out->surfaceType = MST_PLANAR;
	}

	/* set it up */
/*	if (debugSurfaces) {
		out->shaderNum = EmitShader("debugsurfaces", NULL, NULL);
	}
	else{
		out->shaderNum = EmitShader(ds->shaderInfo->shader, &ds->shaderInfo->contentFlags, &ds->shaderInfo->surfaceFlags);
	}
	*/

//	out->patchWidth = ds->patchWidth;
//	out->patchHeight = ds->patchHeight;
//	out->fogNum = ds->fogNum;

	// debug inset (push each triangle vertex towards the center of each triangle it is on 
/*	if (debugInset) 
{
		bspDrawVert_t   *a, *b, *c;
		vec3_t cent, dir;


		// nwalk triangle list 
		for (i = 0; i < ds->numIndexes; i += 3)
		{
			// get verts 
			a = &ds->verts[ds->indexes[i]];
			b = &ds->verts[ds->indexes[i + 1]];
			c = &ds->verts[ds->indexes[i + 2]];

			// calculate centroid 
			VectorCopy(a->xyz, cent);
			VectorAdd(cent, b->xyz, cent);
			VectorAdd(cent, c->xyz, cent);
			VectorScale(cent, 1.0f / 3.0f, cent);

			// offset each vertex 
			VectorSubtract(cent, a->xyz, dir);
			VectorNormalize(dir, dir);
			VectorAdd(a->xyz, dir, a->xyz);
			VectorSubtract(cent, b->xyz, dir);
			VectorNormalize(dir, dir);
			VectorAdd(b->xyz, dir, b->xyz);
			VectorSubtract(cent, c->xyz, dir);
			VectorNormalize(dir, dir);
			VectorAdd(c->xyz, dir, c->xyz);
		}
	}*/

	// RBSP 
/*	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		out->lightmapNum[i] = -3;
		out->lightmapStyles[i] = LS_NONE;
		out->vertexStyles[i] = LS_NONE;
	}
	out->lightmapStyles[0] = LS_NORMAL;
	out->vertexStyles[0] = LS_NORMAL;
*/

	/*
	// lightmap vectors (lod bounds for patches 
	VectorCopy(ds->lightmapOrigin, out->lightmapOrigin);
	VectorCopy(ds->lightmapVecs[0], out->lightmapVecs[0]);
	VectorCopy(ds->lightmapVecs[1], out->lightmapVecs[1]);
	VectorCopy(ds->lightmapVecs[2], out->lightmapVecs[2]);

	// ydnar: gs mods: clear out the plane normal 
	if (ds->planar == qfalse) {
		VectorClear(out->lightmapVecs[2]);
	}
*/

	// optimize the surface's triangles 
//	OptimizeTriangleSurface(ds);

	// emit the verts and indexes 
	EmitDrawVerts(ds, out);
	EmitDrawIndexes(ds, out);

	// add to count 
	numSurfacesByType_[ds->type]++;
}



// ----------------------

void BSPBuilder::EmitDrawVerts(bspDrawSurface* ds, bsp::Surface* out)
{
	int i;

	// copy the verts 
	out->vertexStartIdx = safe_static_cast<int, size_t>(out_.verts.size());
	out->numVerts = ds->numVerts;

	for (i = 0; i < ds->numVerts; i++)
	{
		out_.verts.append(ds->pVerts[i]);
	}
}

void BSPBuilder::EmitDrawIndexes(bspDrawSurface* ds, bsp::Surface* out)
{
	int i;

	// copy indexs
	out->indexStartIdx = safe_static_cast<int,size_t>(out_.indexes.size());
	out->numIndexes = ds->numVerts;

	for (i = 0; i < ds->numIndexes; i++)
	{
		out_.indexes.append(ds->pIndexes[i]);
	}
}