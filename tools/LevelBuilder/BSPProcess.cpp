#include "stdafx.h"
#include "BSPTypes.h"

#include <Containers\FixedArray.h>

#include <IModel.h>

namespace
{

/*

	class AreaMeshBuilder
	{
		typedef core::HashMap<core::string, AreaSubMesh> AreaMeshMap;
	public:
		AreaMeshBuilder() : areaMeshes_(g_arena) {
			areaMeshes_.reserve(4096);
		}
		*/


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


	bool FanFaceSurface(int numVerts, size_t StartVert, AreaSubMesh* pSubmesh)
	{
		int i, j;
		bsp::Vertex *verts, *centroid, *dv;
		float iv;

		verts = &pSubmesh->verts_[StartVert];

		pSubmesh->verts_.insert(bsp::Vertex(), StartVert);

		centroid = &pSubmesh->verts_[StartVert];
		// add up the drawverts to create a centroid 
  		for (i = 1, dv = &verts[1]; i < (numVerts + 1); i++, dv++)
		{
			centroid->pos += dv->pos;
			centroid->normal += dv->normal;
			for (j = 0; j < 4; j++)
			{
				if (j < 2) {
					centroid->texcoord[j] += dv->texcoord[j];
				}
			}
		}

		// average the centroid 
		iv = 1.0f / numVerts;
		centroid->pos *= iv;
		centroid->normal.normalize();

		for (j = 0; j < 4; j++)
		{
			if (j < 2) {
				centroid->texcoord[j] *= iv;
			}
		}

		// add to vert count 
	//	mesh.numVerts++;

		// fill indexes in triangle fan order 
	//	mesh.numIndexes = 0;
		for (i = 1; i < numVerts; i++)
		{
			model::Face face;
			face.x = 0;
			face.y = i;
			face.z = (i + 1) % numVerts;
			face.z = face.z ? face.z : 1;

			pSubmesh->indexes_.append(face);
		}

		return true;
	}


	bool createIndexs(int numVerts, size_t StartVert, AreaSubMesh* pSubmesh)
	{
		X_ASSERT_NOT_NULL(pSubmesh);

		int least;
		int i, r, ni;
		int rotate;
		int numIndexes;
		bsp::Vertex* pVerts;
		core::FixedArray<model::Face, 1024> indexes;

		pVerts = &pSubmesh->verts_[StartVert];

		X_ASSERT_NOT_NULL(pVerts);

		if (numVerts == 0)
		{
			X_WARNING("Bsp", "submesh has zero verts");
			return false;
		}

		// is this a simple triangle? 
		if (numVerts == 3)
		{
			pSubmesh->indexes_.append(model::Face(0, 1, 2));
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
				return false;
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
				return FanFaceSurface(numVerts, StartVert, pSubmesh);
			}
		}

		core::FixedArray<model::Face, 1024>::const_iterator it = indexes.begin();

		for (; it != indexes.end(); ++it)
		{
			pSubmesh->indexes_.append(*it);
		}

		return true;
	}


}



bool LvlBuilder::ProcessModels(void)
{
	int entityNum;
	int numEntities = stats_.numEntities;

	for (entityNum = 0; entityNum < numEntities; entityNum++)
	{
		const LvlEntity& entity = entities_[entityNum];
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



bool LvlBuilder::ProcessModel(const LvlEntity& ent)
{
	


	return true;
}


bool LvlBuilder::ProcessWorldModel(const LvlEntity& ent)
{
	X_LOG0("Bsp", "Processing World Entity");
	bspBrush* pBrush;
	size_t i;
	int x, p;

	// allocate a area.
	// we will have multiple area's for world. (none noob map xD)
	LvlArea& area = areas_.AddOne();

	area.AreaBegin();

	pBrush = ent.pBrushes;
	for (i = 0; i < ent.numBrushes; i++)
	{
		X_ASSERT_NOT_NULL(pBrush);

		for (x = 0; x < pBrush->numsides; x++)
		{
			if (!pBrush->sides[x].pWinding)
				continue;

			const BspSide& side = pBrush->sides[x];
			const XWinding* w = side.pWinding;
			int numPoints = w->GetNumPoints();

			// get areaSubMesh for this material.
			AreaSubMesh* pSubMesh = area.MeshForSide(side, stringTable_);

			size_t StartVert = pSubMesh->verts_.size();

			for (p = 0; p < numPoints; p++)
			{
				bsp::Vertex vert;
				const Vec5f& vec = w->operator[](p);

				vert.pos = vec.asVec3();
				vert.normal = planes[side.planenum].getNormal();
				vert.color = Col_White;
				vert.texcoord[0] = Vec2f(vec.s, vec.t);

				pSubMesh->AddVert(vert);
			}
		
			// create some indexes
			createIndexs(numPoints, StartVert, pSubMesh);
		}
		pBrush = pBrush->next;
	}

	// create the meshes.
	area.AreaEnd();

	if (!area.model.BelowLimits())
		return false;

 	return true;
}



