#include "stdafx.h"
#include "BSPTypes.h"

namespace
{
	static const size_t MAX_INDEXES = 1024;

	#define TINY_AREA   1.0f

	bool IsTriangleDegenerate(bsp::Vertex* points, int a, int b, int c)
	{
		Vec3f v1, v2, v3;
		float d;

		v1 = points[b].pos = points[a].pos;
		v2 = points[c].pos = points[a].pos;
		v3 = v2.cross(v1);
		d = v3.length();


		// assume all very small or backwards triangles will cause problems 
		if (d < TINY_AREA) {
			return true;
		}

		return false;
	}


}

void BSPBuilder::MakeEntityMetaTriangles(const BspEntity& ent)
{
	int i, f, fOld;
	bspDrawSurface    *ds;

	int	numMetaSurfaces = 0;
	int numStripSurfaces = 0;
	int numFanSurfaces = 0;
	int numPatchMetaSurfaces = 0;
	int numMetaVerts = 0;
	int numMetaTriangles = 0;

	X_LOG0("Meta", "--- MakeEntityMetaTriangles ---");

	// init pacifier 
	fOld = -1;

	// walk the list of surfaces in the entity 
	int numMapDrawSurfs = (int)drawSurfs_.size();
	for (i = ent.firstDrawSurf; i < numMapDrawSurfs; i++)
	{
		// print pacifier 
		f = 10 * (i - ent.firstDrawSurf) / (numMapDrawSurfs - ent.firstDrawSurf);
		if (f != fOld) {
			fOld = f;
			X_LOG0("Meta", "%d...", f);
		}

		// get surface 
		ds = &drawSurfs_[i];
		if (ds->numVerts <= 0) {
			continue;
		}


		switch (ds->type)
		{
			case DrawSurfaceType::FACE:
				StripFaceSurface(ds);
				SurfaceToMetaTriangles(ds);
			break;

			default:
			break;
		}
	}

	if ((numMapDrawSurfs - ent.firstDrawSurf)) {
	//	X_LOG0("Meta", " (%d)\n", (int)(I_FloatTime() - start));
	}

	X_LOG0("Meta", "%9d total meta surfaces", numMetaSurfaces);
	X_LOG0("Meta", "%9d stripped surfaces", numStripSurfaces);
	X_LOG0("Meta", "%9d fanned surfaces", numFanSurfaces);
	X_LOG0("Meta", "%9d patch meta surfaces", numPatchMetaSurfaces);
	X_LOG0("Meta", "%9d meta verts", numMetaVerts);
	X_LOG0("Meta", "%9d meta triangles", numMetaTriangles);


	TidyEntitySurfaces(ent);
}

void BSPBuilder::TidyEntitySurfaces(const BspEntity& ent)
{

}

void BSPBuilder::StripFaceSurface(bspDrawSurface* ds)
{
	int least;
	int i, r, ni;
	int a, b, c;
	int rotate;
	int numIndexes;
	int indexes[MAX_INDEXES];
//	float       *v1, *v2;


	if (!ds->numVerts || (ds->type != DrawSurfaceType::FACE && ds->type != DrawSurfaceType::DECAL))
		return;
	

	// is this a simple triangle? 
	if (ds->numVerts == 3) 
	{
		numIndexes = 3;
		indexes[0] = 0;
		indexes[0] = 1;
		indexes[0] = 2;
	}
	else
	{
		
#if 1
		//  find smallest coordinate 
		least = 0;
		/*
		if (ds->shaderInfo != nullptr && ds->shaderInfo->autosprite == false) 
		{
			for (i = 0; i < ds->numVerts; i++)
			{
				// get points 
				v1 = ds->verts[i].xyz;
				v2 = ds->verts[least].xyz;

				// compare 
				if (v1[0] < v2[0] ||
					(v1[0] == v2[0] && v1[1] < v2[1]) ||
					(v1[0] == v2[0] && v1[1] == v2[1] && v1[2] < v2[2])) {
					least = i;
				}
			}
		}
		*/

		// determine the triangle strip order 
		numIndexes = (ds->numVerts - 2) * 3;
		if (numIndexes > MAX_INDEXES) 
		{
			X_ERROR("Meta","MAX_INDEXES exceeded for surface (%d > %d) (%d verts)",
				numIndexes, MAX_INDEXES, ds->numVerts);
		}

		// try all possible orderings of the points looking for a non-degenerate strip order 
		for (r = 0; r < ds->numVerts; r++)
		{
			// set rotation 
			rotate = (r + least) % ds->numVerts;

			// walk the winding in both directions 
			for (ni = 0, i = 0; i < ds->numVerts - 2 - i; i++)
			{
				// make indexes 
				a = (ds->numVerts - 1 - i + rotate) % ds->numVerts;
				b = (i + rotate) % ds->numVerts;
				c = (ds->numVerts - 2 - i + rotate) % ds->numVerts;

				// test this triangle 
				if (ds->numVerts > 4 && IsTriangleDegenerate(ds->pVerts, a, b, c)) {
					break;
				}
				indexes[ni++] = a;
				indexes[ni++] = b;
				indexes[ni++] = c;

				// handle end case 
				if (i + 1 != ds->numVerts - 1 - i) 
				{
					// make indexes 
					a = (ds->numVerts - 2 - i + rotate) % ds->numVerts;
					b = (i + rotate) % ds->numVerts;
					c = (i + 1 + rotate) % ds->numVerts;

					// test triangle 
					if (ds->numVerts > 4 && IsTriangleDegenerate(ds->pVerts, a, b, c)) {
						break;
					}
					indexes[ni++] = a;
					indexes[ni++] = b;
					indexes[ni++] = c;
				}
			}

			// valid strip? 
			if (ni == numIndexes) {
				break;
			}
		}

		// if any triangle in the strip is degenerate, render from a centered fan point instead 
		if (ni < numIndexes) 
		{
			X_ASSERT_NOT_IMPLEMENTED();
			//FanFaceSurface(ds);
			return;
		}
#endif
	}

	// copy strip triangle indexes 
	ds->numIndexes = numIndexes;
	ds->pIndexes = X_NEW_ARRAY(bsp::Index, ds->numIndexes, g_arena, "MetaIndexes");

	memcpy(ds->pIndexes, indexes, ds->numIndexes * sizeof(int));

	// add to count 
//	numStripSurfaces++;

	// classify it 
//	ClassifySurfaces(1, ds);

}

void BSPBuilder::SurfaceToMetaTriangles(bspDrawSurface* ds)
{

}