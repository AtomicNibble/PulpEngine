#include "stdafx.h"
#include "BSPTypes.h"

#include <Containers\FixedArray.h>

#include <IModel.h>

namespace
{

	// ---------------------------------------------
	static const size_t MAX_INDEXES = 1024;

#define TINY_AREA   1.0f

	bool IsTriangleDegenerate(bsp::Vertex* points, const model::Face& face)
	{
		Vec3f v1, v2, v3;
		float d;

		v1 = points[face.y].pos = points[face.x].pos;
		v2 = points[face.z].pos = points[face.x].pos;
		v3 = v2.cross(v1);
		d = v3.length();


		// assume all very small or backwards triangles will cause problems 
		if (d < TINY_AREA) {
			return true;
		}

		return false;
	}


	bool FanFaceSurface(AreaModel* pArea, model::SubMeshHeader& mesh)
	{
		int i, j;
		bsp::Vertex *verts, *centroid, *dv;
		float iv;

		int numVerts = mesh.numVerts;

		verts = &pArea->verts[mesh.startVertex];

		pArea->verts.insert(bsp::Vertex(), mesh.startVertex);

		centroid = &pArea->verts[mesh.startVertex];
		// add up the drawverts to create a centroid 
  		for (i = 1, dv = &verts[1]; i < (numVerts + 1); i++, dv++)
		{
			centroid->pos += dv->pos;
			centroid->normal += dv->normal;
			for (j = 0; j < 4; j++)
			{
				if (j < 2) {
			//		centroid->st[j] += dv->st[j];
				}
			}
		}

		// average the centroid 
		iv = 1.0f / numVerts;
		centroid->pos *= iv;
		centroid->normal.normalize();
	//	if (centroid->normal.normalize() <= 0) {
	//		VectorCopy(verts[1].normal, centroid->normal);
	//	}
		for (j = 0; j < 4; j++)
		{
			if (j < 2) {
	//			centroid->st[j] *= iv;
			}
		}

		// add to vert count 
		mesh.numVerts++;

		// fill indexes in triangle fan order 
		mesh.numIndexes = 0;
		for (i = 1; i < mesh.numVerts; i++)
		{
			model::Face face;
			face.x = 0;
			face.y = i;
			face.z = (i + 1) % mesh.numVerts;
			face.z = face.z ? face.z : 1;

			pArea->indexes.append(face);
		}

		return true;
	}


	bool createIndexs(AreaModel* pArea, model::SubMeshHeader& mesh)
	{
		X_ASSERT_NOT_NULL(pArea);

		int least;
		int i, r, ni;
		int rotate;
		int numIndexes;
		int numVerts;
		bsp::Vertex* pVerts;
		core::FixedArray<model::Face, 1024> indexes;

		numVerts = mesh.numVerts;
		pVerts = &pArea->verts[mesh.startVertex];

		X_ASSERT_NOT_NULL(pVerts);

		if (numVerts == 0)
		{
			X_WARNING("Bsp", "submesh has zero verts");
			return false;
		}

		// is this a simple triangle? 
		if (numVerts == 3)
		{
			pArea->indexes.append(model::Face(0, 1, 2));
			mesh.numIndexes = 3;
			return true;
		}
		else
		{
			least = 0;

			// determine ho many indexs are needed.
			numIndexes = (numVerts - 2) * 3;

			if (numIndexes > indexes.capacity())
			{
				X_ERROR("Meta", "MAX_INDEXES exceeded for surface (%d > %d) (%d verts)",
					numIndexes, indexes.capacity(), numVerts);
			}

			// try all possible orderings of the points looking for a non-degenerate strip order 
			for (r = 0; r < numVerts; r++)
			{
				// set rotation 
				rotate = (r + least) % numVerts;

				// walk the winding in both directions 
				for (ni = 0, i = 0; i < numVerts - 2 - i; i++)
				{
					// make indexes 
					model::Face face;
					face.x = (numVerts - 1 - i + rotate) % numVerts;
					face.y = (i + rotate) % numVerts;
					face.z = (numVerts - 2 - i + rotate) % numVerts;

					// test this triangle 
					if (numVerts > 4 && IsTriangleDegenerate(pVerts, face)) {
						break;
					}
					indexes.append(face);

					// handle end case 
					if (i + 1 != numVerts - 1 - i)
					{
						// make indexes 
						face.x = (numVerts - 2 - i + rotate) % numVerts;
						face.y = (i + rotate) % numVerts;
						face.z = (i + 1 + rotate) % numVerts;

						// test triangle 
						if (numVerts > 4 && IsTriangleDegenerate(pVerts, face)) {
							break;
						}

						indexes.append(face);
					}
				}

				// valid strip? 
				if ((indexes.size() * 3) == numIndexes) {
					break;
				}

				indexes.clear();
			}

			// if any triangle in the strip is degenerate, render from a centered fan point instead 
			if ((indexes.size() * 3) < numIndexes)
			{
				return FanFaceSurface(pArea, mesh);
			}
		}

		core::FixedArray<model::Face, 1024>::const_iterator it = indexes.begin();

		for (; it != indexes.end(); ++it)
		{
			pArea->indexes.append(*it);
		}

		mesh.numIndexes = safe_static_cast<uint32_t,size_t>(indexes.size() * 3);
		return true;
	}


}

bool BSPBuilder::ProcessModels(void)
{
	int entityNum;
	int numEntities = stats_.numEntities;

	for (entityNum = 0; entityNum < numEntities; entityNum++)
	{
		const LvlEntity& entity = entities[entityNum];
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



bool BSPBuilder::ProcessModel(const LvlEntity& ent)
{
	


	return true;
}




bool BSPBuilder::ProcessWorldModel(const LvlEntity& ent)
{
	X_LOG0("Bsp", "Processing World Entity");
	AreaModel* pArea;
	bspBrush* pBrush;
	size_t i;
	int x, p;

	pArea = X_NEW(AreaModel, g_arena, "AreaModel");
	pArea->BeginModel(ent);

	areaModels.append(pArea);
	
	// Split the map via portals.
	// each area is then turned into a model.
	// Each brush is made into a subMesh.
	// and the verts + indexes added to the models buffer.

	pBrush = ent.pBrushes;

	for (i = 0; i < ent.numBrushes; i++)
	{
		X_ASSERT_NOT_NULL(pBrush);
		for (x = 0; x < pBrush->numsides; x++)
		{
			if (!pBrush->sides[x].pWinding)
				continue;

			model::SubMeshHeader mesh;

			mesh.boundingBox = pBrush->bounds;
			mesh.boundingSphere = Sphere(pBrush->bounds);
			mesh.startVertex = safe_static_cast<uint32_t, size_t>(pArea->verts.size());
			mesh.startIndex = safe_static_cast<uint32_t, size_t>(pArea->indexes.size() * 3);

			const BspSide& side = pBrush->sides[x];
			const XWinding* w = side.pWinding;

			int numPoints = w->GetNumPoints();
			for (p = 0; p < numPoints; p++)
			{
				bsp::Vertex vert;
				const Vec5f& vec = w->operator[](p);;

				vert.pos = vec.asVec3();
				vert.normal = planes[side.planenum].getNormal();
				vert.color = Col_White;
				vert.texcoord[0] = Vec2f(vec.s,vec.t);

				pArea->verts.append(vert);
			}

			mesh.numVerts = numPoints;

			// create some indexes
			createIndexs(pArea, mesh);

			pArea->meshes.append(mesh);
		}	

		pBrush = pBrush->next;
	}
	
	if (!pArea->BelowLimits())
		return false;

	pArea->EndModel();

 	return true;
}



