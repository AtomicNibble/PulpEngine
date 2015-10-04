#include "stdafx.h"
#include "LvlBuilder.h"
#include "ModelInfo.h"

#include <Containers\FixedArray.h>
#include <IModel.h>

#include "MapTypes.h"
#include "MapLoader.h"

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

		X_LOG0("Entity", "^5----- entity %i -----", i);

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
	X_ASSERT_NOT_IMPLEMENTED();


	return true;
}


bool LvlBuilder::LoadDefaultModel(void)
{
	if (!ModelInfo::GetNModelAABB(X_CONST_STRING("default"), defaultModelBounds_))
	{
		X_ERROR("","Failed to load default model info");
		return false;
	}
	return true;
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

void LvlBuilder::PutWindingIntoAreas_r(LvlEntity& ent, XWinding* pWinding,
	LvlBrushSide& side, bspNode* pNode)
{
	X_ASSERT_NOT_NULL(pNode);
	XWinding *front, *back;


	if (!pWinding) {
		return;
	}

	if (pNode->planenum != PLANENUM_LEAF)
	{
		if (side.planenum == pNode->planenum) {
			PutWindingIntoAreas_r(ent, pWinding, side, pNode->children[0]);
			return;
		}
		if (side.planenum == (pNode->planenum ^ 1)) {
			PutWindingIntoAreas_r(ent, pWinding, side, pNode->children[1]);
			return;
		}

		pWinding->Split(planes[pNode->planenum], 
			ON_EPSILON, &front, &back, g_arena);

		PutWindingIntoAreas_r(ent, front, side, pNode->children[0]);
		if (front) {
			X_DELETE(front, g_arena);
		}

		PutWindingIntoAreas_r(ent, back, side, pNode->children[1]);
		if (back) {
			X_DELETE(back, g_arena);
		}

		return;
	}

	// if opaque leaf, don't add
	if (pNode->opaque) {
		return;
	}

	// valid area?
	if (pNode->area == -1) {
		return;
	}

	// skip null materials
	if (!side.matInfo.pMaterial) {
		X_WARNING("Lvl", "side without a material");
		return;
	}

	// skip none visable materials.
	if (!side.matInfo.pMaterial->isDrawn()) {
		X_LOG1("Lvl", "Skipping visible face, material not drawn: \"%s\"",
			side.matInfo.name.c_str());
		return;
	}


	// now we add the side to the area index of the node.
	LvlArea& area = areas_[pNode->area];

	// get areaSubMesh for this material.
	AreaSubMesh* pSubMesh = area.MeshForSide(side, stringTable_);

	size_t StartVert = pSubMesh->verts_.size();

	int numPoints = pWinding->getNumPoints();

#if 0
	int p;
	for (p = 0; p < numPoints; p++)
	{
		level::Vertex vert;
		const Vec5f& vec = pWinding->operator[](p);

		vert.pos = vec.asVec3();
		vert.normal = planes[side.planenum].getNormal();
		vert.color = Col_White;
		vert.texcoord[0] = Vec2f(vec.s, vec.t);

		pSubMesh->AddVert(vert);
	}
	// create some indexes
	createIndexs(numPoints, StartVert, pSubMesh);

#else

	const XWinding* w = pWinding;
	int i, j;

	model::Index offset = safe_static_cast<model::Index, size_t>(StartVert);

	for (i = 2; i < numPoints; i++)
	{
		for (j = 0; j < 3; j++)
		{
			level::Vertex vert;

			if (j == 0) {
				const Vec5f vec = (*w)[0];
				vert.pos = vec.asVec3();
				vert.texcoord[0] = Vec2f(vec.s, vec.t);
			}
			else if (j == 1) {
				const Vec5f vec = (*w)[i - 1];
				vert.pos = vec.asVec3();
				vert.texcoord[0] = Vec2f(vec.s, vec.t);
			}
			else
			{
				const Vec5f vec = (*w)[i];
				vert.pos = vec.asVec3();
				vert.texcoord[0] = Vec2f(vec.s, vec.t);
			}

			// copy normal
			vert.normal = planes[side.planenum].getNormal();
			vert.color = Col_White;

			pSubMesh->AddVert(vert);
		}

		model::Face face(0,1,2);

		face += model::Face(offset, offset, offset);

		model::Index localOffset = safe_static_cast<model::Index, size_t>((i - 2) * 3);

		face += model::Face(localOffset, localOffset, localOffset);

		pSubMesh->faces_.append(face);
	}
#endif


	int goat = 0;
}


void LvlBuilder::AddAreaRefs_r(core::Array<int32_t>& areaList, const Sphere& sphere,
	const Vec3f boundsPoints[8], bspNode* pNode)
{
	X_ASSERT_NOT_NULL(boundsPoints);
	X_ASSERT_NOT_NULL(pNode);
	bspNode* pCurNode = pNode;

	if (pCurNode->IsAreaLeaf()) 
	{
		int32_t areaIdx = pCurNode->area;

		// check if duplicate.
		if (areaList.isEmpty()) {
			areaList.append(areaIdx);
		}
		else
		{
			// just linear search as it will be fastest with such low numbers.
			// and contigous memory.
			size_t i;
			for (i = 0; i < areaList.size(); i++) {
				if (areaList[i] == areaIdx) {
					break;
				}
			}
			if (i == areaList.size()) {
				areaList.append(areaIdx);
			}
		}

		return;
	}

	const Planef& plane = planes[pCurNode->planenum];
	float sd = plane.distance(sphere.center());

	if (sd >= sphere.radius())
	{
		pCurNode = pCurNode->children[0];
		if (!pCurNode->IsSolidLeaf()) {
			AddAreaRefs_r(areaList, sphere, boundsPoints, pCurNode);
		}
		return;
	}
	if (sd <= -sphere.radius())
	{
		pCurNode = pCurNode->children[1];
		if (!pCurNode->IsSolidLeaf()) {
			AddAreaRefs_r(areaList, sphere, boundsPoints, pCurNode);
		}
		return;
	}

	// check bounds points.
	bool front = false;
	bool back = false;
	for (size_t i = 0; i < 8; i++)
	{
		float d = plane.distance(boundsPoints[i]);

		if (d >= 0.0f) {
			front = true;
		}
		else if (d <= 0.0f) {
			back = true;
		}
		if (back && front) {
			break;
		}
	}

	if (front) {
		bspNode* frontChild = pCurNode->children[0];
		if (!frontChild->IsSolidLeaf()) {
			AddAreaRefs_r(areaList, sphere, boundsPoints, frontChild);
		}
	}
	if (back) {
		bspNode* backChild = pCurNode->children[1];
		if (!backChild->IsSolidLeaf()) {
			AddAreaRefs_r(areaList, sphere, boundsPoints, backChild);
		}
	}
}

bool LvlBuilder::CreateEntAreaRefs(LvlEntity& worldEnt)
{
	int32_t i, numEnts;

	// we go throught each ent, and work out what area's it is in.
	// each ent is then added to the entRefts set.
	// we then need to work out the ones that touch multiple area's
	core::Array<int32_t> areaList(g_arena);
	areaList.resize(this->areas_.size());

	const size_t maxMultiRefLists = (level::MAP_MAX_AREAS / 32);
	core::Array<level::MultiAreaEntRef> multiRefLists[maxMultiRefLists] = { g_arena,
		g_arena, g_arena, g_arena, g_arena, g_arena, g_arena, g_arena };


	numEnts = map_->getNumEntities();
	for (i = 0; i < numEnts; i++)
	{
		mapfile::XMapEntity* mapEnt = map_->getEntity(i);
		LvlEntity& lvlEnt = entities_[i];
		
		if (lvlEnt.classType != level::ClassType::MISC_MODEL) {
			continue;
		}

		// for now just add the static models to area ref's
#if 1
		{
			mapfile::XMapEntity::PairIt it;

			it = mapEnt->epairs.find(X_CONST_STRING("model"));
			if (it == mapEnt->epairs.end()) 
			{
				X_WARNING("Entity", "misc_model missing 'model' kvp at: (^8%g,%g,%g^7)",
					lvlEnt.origin[0], lvlEnt.origin[1], lvlEnt.origin[2]);
				continue;
			}

			const core::string& modelName = it->second;
			X_LOG0("Entity", "Ent model: \"%s\"", modelName.c_str());
		}
#endif

		// find out what areas the bounds are in.
		// then add a refrence for that end to the area.

		AABB worldBounds;
		worldBounds.set(lvlEnt.bounds.min + lvlEnt.origin,
			lvlEnt.bounds.max + lvlEnt.origin);

		// make a sphere, for quicker testing.
		Sphere worldSphere(worldBounds);

		// get the the worldBounds points for more accurate testing if sphere test pass.
		Vec3f boundsPoints[8];
		worldBounds.toPoints(boundsPoints);

		// clear from last time.
		areaList.clear();

		// traverse the world ent's tree
		AddAreaRefs_r(areaList, worldSphere, boundsPoints, worldEnt.bspTree.headnode);

		size_t numRefs = areaList.size();
		if (numRefs)
		{
			X_LOG0("Lvl", "Entity(%i) has %i refs", i, numRefs);

			// ok so we hold a list of unique areas ent is in.
			if (numRefs == 1)
			{
				// add to area's ref list.
				LvlArea& area = this->areas_[areaList[0]];

				area.entRefs.push_back(i);
			}
			else
			{
				// added to the multiAreaRefList.
				uint32_t flags[maxMultiRefLists] = { 0 };

				auto it = areaList.begin();
				for (; it != areaList.end(); ++it)
				{
					const int32_t areaIdx = *it;
					// work out what area list.
					const size_t areaListIdx = (areaIdx / 32);

					flags[areaListIdx] |= (1 << (areaIdx % 32));
				}

				level::MultiAreaEntRef entRef;
				entRef.entId = i;
				for (size_t x = 0; x < maxMultiRefLists; x++)
				{
					if (flags[x] != 0)
					{
						entRef.flags = flags[x];
						multiRefLists[x].append(entRef);
					}
				}

			}
		}
		else
		{
			// ent not in any area.
			X_ERROR("Lvl", "Entity(%i) does not reside in any area: (%g,%g,%g)",
				i, lvlEnt.origin.x, lvlEnt.origin.y, lvlEnt.origin.z);
		}
	}

	return true;
}

bool LvlBuilder::PutPrimitivesInAreas(LvlEntity& ent)
{
	X_LOG0("Lvl", "^5----- PutPrimitivesInAreas -----");

	// ok now we must create the areas and place the primatives into each area.
	// clip into non-solid leafs and divide between areas.
	size_t i, j;

	areas_.resize(ent.numAreas);
	for (i = 0; i < areas_.size(); i++){
		areas_[i].AreaBegin();
	}

	for (i = 0; i < ent.brushes.size(); i++)
	{
		LvlBrush& brush = ent.brushes[i];
		// for each side that's visable.
		
		for (j = 0; j < brush.sides.size(); j++)
		{ 
			LvlBrushSide& side = brush.sides[j];
		
			if (!side.pVisibleHull) {
				continue;
			}

			PutWindingIntoAreas_r(ent, side.pVisibleHull, side, ent.bspTree.headnode);
		}
	}
	
	for (i = 0; i < areas_.size(); i++){
		areas_[i].AreaEnd();

		if (!areas_[i].model.BelowLimits()) {
			X_ERROR("Lvl","Area %i exceeds the limits", i);
			return false;
		}
	}

	return true;
}



bool LvlBuilder::ProcessWorldModel(LvlEntity& ent)
{
	X_LOG0("Lvl", "Processing World Entity");

	if (ent.classType != level::ClassType::WORLDSPAWN) {
		X_ERROR("Lvl", "World model is missing class name: 'worldspawn'");
		return false;
	}

	// make structural face list.
	// which is the planes and windings of all the structual faces.
	// Portals become part of this.
	ent.MakeStructuralFaceList();
	// we create a tree from the FaceList
	// this is done by spliting the nodes multiple time.
	// we end up with a binary tree.
	ent.FacesToBSP(planes);
	// next we want to make a portal that covers the whole map.
	// this is the outside_node of the bspTree
	ent.MakeTreePortals(planes);
	// Mark the leafs as opaque and areaportals and put brush
	// fragments in each leaf so portal surfaces can be matched
	// to materials
	ent.FilterBrushesIntoTree(planes);

	// take the entities and use them to floor the node portals.
	// so that all inside leafs are marked.
	if (!ent.FloodEntities(planes, entities_, map_)) {
		X_ERROR("LvlEntity", "leaked");
		return false;
	}
	// anything that is not inside or opaque must
	// be none visable / outside.
	ent.FillOutside();
	// get minimum convex hulls for each visible side
	// this must be done before creating area portals,
	// because the visible hull is used as the portal
	ent.ClipSidesByTree(planes);
	// determine areas before clipping tris into the
	// tree, so tris will never cross area boundaries
	ent.FloodAreas();
	// we now have a BSP tree with solid and non-solid leafs marked with areas
	// all primitives will now be clipped into this, throwing away
	// fragments in the solid areas
	PutPrimitivesInAreas(ent);

	// prune the nodes, so that we only have one leaf per a area.
	// we also number the nodes at this point also.
	ent.PruneNodes();


	// we want to create a list of static models for the level.
	// we then work out a ref list for each area.
	// so I know what models are in each area.

	// work out which ents belong to which area.
//	ent.PutEntsInAreas(planes, entities_, map_);
	CreateEntAreaRefs(ent);
 	return true;
}



