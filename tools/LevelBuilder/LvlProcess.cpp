#include "stdafx.h"
#include "LevelBuilder.h"

#include <Containers\FixedArray.h>
#include <IModel.h>

namespace
{
	// ---------------------------------------------
	static const size_t MAX_INDEXES = 1024;

	#define TINY_AREA   1.0f

	bool IsTriangleDegenerate(level::Vertex* points, const model::Face& face)
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
		level::Vertex *verts, *centroid, *dv;
		float iv;

		verts = &pSubmesh->verts_[StartVert];

		pSubmesh->verts_.insert(level::Vertex(), StartVert);

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

		for (i = 1; i < numVerts; i++)
		{
			model::Face face;
			face.x = 0;
			face.y = i;
			face.z = (i + 1) % numVerts;
			face.z = face.z ? face.z : 1;

			pSubmesh->faces_.append(face);
		}

		return true;
	}


	bool createIndexs(int numVerts, size_t StartVert, AreaSubMesh* pSubmesh)
	{
		X_ASSERT_NOT_NULL(pSubmesh);

		int least;
		int i, r, ni;
		int rotate;
		size_t numIndexes;
		level::Vertex* pVerts;
		core::FixedArray<model::Face, 1024> indexes;

		pVerts = &pSubmesh->verts_[StartVert];

		X_ASSERT_NOT_NULL(pVerts);

		if (numVerts == 0)
		{
			X_WARNING("Lvl", "submesh has zero verts");
			return false;
		}

		// is this a simple triangle? 
		if (numVerts == 3)
		{
			pSubmesh->faces_.append(model::Face(0, 1, 2));
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

		model::Index offset = safe_static_cast<model::Index, size_t>(StartVert);

		for (; it != indexes.end(); ++it)
		{
			model::Face face = *it;

			face += model::Face(offset, offset, offset);

			pSubmesh->faces_.append(face);
		}

		return true;
	}


}



bool LvlBuilder::ProcessModels(void)
{
	size_t i, numEnts = entities_.size();

	for (i = 0; i < numEnts; i++)
	{
		LvlEntity& entity = entities_[i];
		if (entity.brushes.isEmpty()) {
			continue;
		}

		X_LOG0("Entity", "----------- entity %i -----------", i);

		if (i == 0)
		{
			// return false if leak.
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



bool LvlBuilder::ProcessModel(LvlEntity& ent)
{
	


	return true;
}


void LvlBuilder::MakeStructuralFaceList(LvlEntity& ent)
{
	X_LOG0("Lvl", "Processing World Entity");
	size_t i, x;

	for (i = 0; i < ent.brushes.size(); i++)
	{
		LvlBrush& brush = ent.brushes[i];

		if (!brush.opaque)
		{
			// if it's not opaque and none of the sides are portals it can't be structual.
			if (!brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL))
			{
				continue;
			}
		}

		for (x = 0; x < brush.sides.size(); x++)
		{
			LvlBrushSide& side = brush.sides[x];

			if (!side.pWinding) {
				continue;
			}

			// if combined flags are portal, check what this side is.
			if (brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL))
			{
				engine::IMaterial* pMaterial = side.matInfo.pMaterial;
				X_ASSERT_NOT_NULL(pMaterial);

				engine::MaterialFlags flags = pMaterial->getFlags();

				if (!flags.IsSet(engine::MaterialFlag::PORTAL))
				{
					continue;
				}
			}

			bspFace* pFace = X_NEW(bspFace, g_arena, "BspFace");
			pFace->planenum = side.planenum & ~1;
			pFace->w = side.pWinding->Copy();
			pFace->pNext = ent.bspFaces;
			ent.bspFaces = pFace;
		}
	}
}

void LvlBuilder::calculateLvlBounds(void)
{
	mapBounds.clear();

	// bound me baby
	LvlEntsArr::ConstIterator it = entities_.begin();
	for (; it != entities_.end(); ++it) 
	{
		LvlEntity::LvlBrushArr::ConstIterator bIt = it->brushes.begin();
		LvlEntity::LvlBrushArr::ConstIterator bEnd = it->brushes.end();
		for (; bIt != bEnd; ++bIt)
		{
			const LvlBrush& brush = *bIt;

			mapBounds.add(brush.bounds);
		}
	}
}


void LvlBuilder::ClipSideByTree_r(XWinding* w, LvlBrushSide& side, bspNode *node)
{
	XWinding		*front, *back;

	if (!w) {
		return;
	}

	if (node->planenum != PLANENUM_LEAF)
	{
		if (side.planenum == node->planenum) {
			ClipSideByTree_r(w, side, node->children[0]);
			return;
		}
		if (side.planenum == (node->planenum ^ 1)) {
			ClipSideByTree_r(w, side, node->children[1]);
			return;
		}

		w->Split(planes[node->planenum], ON_EPSILON, &front, &back);
	
		X_DELETE(w, g_arena);

		ClipSideByTree_r(front, side, node->children[0]);
		ClipSideByTree_r(back, side, node->children[1]);

		return;
	}

	// if opaque leaf, don't add
	if (!node->opaque) {
		if (!side.pVisibleHull) {
			side.pVisibleHull = w->Copy();
		}
		else {
			side.pVisibleHull->AddToConvexHull(w, planes[side.planenum].getNormal());
		}
	}

	X_DELETE(w, g_arena);
	return;


}

bool LvlBuilder::ClipSidesByTree(LvlEntity& ent)
{
	X_LOG0("Lvl", "--- ClipSidesByTree ---");

	size_t i, x;

	for (i = 0; i < ent.brushes.size(); i++)
	{
		LvlBrush& brush = ent.brushes[i];

		for (x = 0; x < brush.sides.size(); x++)
		{
			LvlBrushSide& side = brush.sides[x];

			if (!side.pWinding) {
				continue;
			}

			if (side.pVisibleHull) {
				X_ERROR("Lvl","Visable hull already set");
				return false;
			}

			XWinding* w = side.pWinding->Copy();

			ClipSideByTree_r(w, side, ent.bspTree.headnode);

		}
	}

	return true;
}

bool LvlBuilder::FloodAreas(LvlEntity& ent)
{
	X_LOG0("Lvl", "--- FloodAreas ---");

	size_t numAreas = 0;

	FindAreas_r(ent.bspTree.headnode, numAreas);

	X_LOG0("Lvl", "%5i areas", numAreas);

	ent.numAreas = numAreas;

	return true;
}


/*
=============
Portal_Passable

Returns true if the portal has non-opaque leafs on both sides
=============
*/
static bool Portal_Passable(bspPortal  *p) 
{
	if (!p->onNode) {
		return false;	// to global outsideleaf
	}

	if (p->nodes[0]->planenum != PLANENUM_LEAF
		|| p->nodes[1]->planenum != PLANENUM_LEAF) 
	{
		X_ERROR("lvl","Portal_EntityFlood: not a leaf");
	}

	if (!p->nodes[0]->opaque && !p->nodes[1]->opaque) {
		return true;
	}

	return false;
}

LvlBrushSide* FindSideForPortal(bspPortal* p) 
{
	size_t			i, x, j, k;
	bspNode			*node;
	LvlBrush		*b, *orig;
	LvlBrushSide	*s2;

	// scan both bordering nodes brush lists for a portal brush
	// that shares the plane
	for (i = 0; i < 2; i++) 
	{
		node = p->nodes[i];
		node->brushes.size();
		for (x = 0; x < node->brushes.size(); x++)
		{
			b = node->brushes[x];

			// do we have a side with a portal?
			if (!b->combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL)) {
				continue;
			}

			orig = b->pOriginal;

			// iterate the sides to find the portals.
			// b->sides
			for (j = 0; j < b->sides.size(); j++)
			{
				LvlBrushSide& side = b->sides[j];

				// must be visable.
				if (!side.pVisibleHull) {
					continue;
				}

				// portal?
				{
					engine::IMaterial* pMaterial = side.matInfo.pMaterial;
					X_ASSERT_NOT_NULL(pMaterial);

					if (!pMaterial->getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
						continue;
					}
				}

				if ((side.planenum & ~1) != (p->onNode->planenum & ~1)) {
					continue;
				}

				// remove the visible hull from any other portal sides of this portal brush
				for (k = 0; k < b->sides.size(); k++)
				{
					// skip self
					if (k == j) {
						continue;
					}

					s2 = &orig->sides[k];

					if (s2->pVisibleHull == nullptr) {
						continue;
					}

					// portal side?
					{
						engine::IMaterial* pMaterial = s2->matInfo.pMaterial;
						X_ASSERT_NOT_NULL(pMaterial);

						if (!pMaterial->getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
							continue;
						}
					}

					Vec3f center = s2->pVisibleHull->GetCenter();

					X_WARNING("Portal","brush has multiple area portal sides at (%g,%g,%g)",
						center[0], center[1], center[2]);

					X_DELETE_AND_NULL( s2->pVisibleHull, g_arena);
				}
				return &side;
			}
		}
	}
	return nullptr;
}

void FloodAreas_r(bspNode *node, size_t area, size_t& areaFloods)
{
	bspPortal	*p;
	int			s;

	if (node->area != -1) {
		return;	// allready got it
	}
	if (node->opaque) {
		return;
	}

	areaFloods++;
	node->area = area;

	for (p = node->portals; p; p = p->next[s])
	{
		bspNode	*other;

		s = (p->nodes[1] == node);
		other = p->nodes[!s];

		if (!Portal_Passable(p)) {
			continue;
		}

		// can't flood through an area portal
		if (FindSideForPortal(p)) {
			continue;
		}

		FloodAreas_r(other, area, areaFloods);
	}
}

void LvlBuilder::FindAreas_r(bspNode* node, size_t& numAreas)
{
	if (node->planenum != PLANENUM_LEAF) {
		FindAreas_r(node->children[0], numAreas);
		FindAreas_r(node->children[1], numAreas);
		return;
	}

	if (node->opaque) {
		return;
	}

	if (node->area != -1) {
		return;	// allready got it
	}

	size_t areaFloods = 0;
	FloodAreas_r(node, numAreas, areaFloods);

	X_LOG0("Lvl", "area %i has %i leafs", numAreas, areaFloods);
	numAreas++;
}

bool LvlBuilder::PutPrimitivesInAreas(LvlEntity& ent)
{

	return true;
}

bool LvlBuilder::ProcessWorldModel(LvlEntity& ent)
{
	X_LOG0("Lvl", "Processing World Entity");

	// make structural face list.
	// which is the planes and windings of all the structual faces.
	// Portals become part of this.
	MakeStructuralFaceList(ent);
	// we create a tree from the FaceList
	// this is done by spliting the nodes multiple time.
	// we end up with a binary tree.
	FacesToBSP(ent);

	// next we want to make a portal that covers the whole map.
	// this is the outside_node of the bspTree
	MakeTreePortals(ent);

	// Mark the leafs as opaque and areaportals and put brush
	// fragments in each leaf so portal surfaces can be matched
	// to materials
	FilterBrushesIntoTree(ent);


	if (!FloodEntities(ent))
	{
		X_ERROR("Lvl", "map leaked");
		return false;
	}

	FillOutside(ent);


	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ClipSidesByTree(ent); 

	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	FloodAreas(ent);

	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas(ent);

	int goat = 0;

#if 0
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
				level::Vertex vert;
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
#endif
 	return true;
}



