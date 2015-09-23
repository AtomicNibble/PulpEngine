#include "stdafx.h"
#include "BSPTypes.h"
#include "LvlTypes.h"

#include "FaceTreeBuilder.h"

#include "MapTypes.h"
#include "MapLoader.h"

// ===================


bool LvlEntity::FindInterAreaPortals(void)
{
	if (!bspTree.headnode) {
		X_ERROR("LvlEnt", "Can't find inter area portal information. tree is invalid.");
		return false;
	}

	return FindInterAreaPortals_r(bspTree.headnode);
}

bool LvlEntity::FindInterAreaPortals_r(bspNode* node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		if (!FindInterAreaPortals_r(node->children[0]))
			return false;
		if (!FindInterAreaPortals_r(node->children[1]))
			return false;

		return true;
	}

	// skip opaque.
	if (node->opaque) {
		return true;
	}

	// iterate the nodes portals.
	size_t s;
	bspPortal* p = nullptr;
	const LvlBrushSide* pBSide = nullptr;
	XWinding* w = nullptr;
	bspNode* pOther = nullptr;

	for (p = node->portals; p; p = p->next[s])
	{
		s = (p->nodes[1] == node);
		pOther = p->nodes[!s];

		if (pOther->opaque) {
			continue;
		}

		// only report areas going from lower number to higher number
		// so we don't report the portal twice
		if (pOther->area <= node->area) {
			continue;
		}

		pBSide = p->FindAreaPortalSide();
		if (!pBSide)
		{
			Vec3f center = p->pWinding->getCenter();
			X_ERROR("LvlEnt", "Failed to find portal side for inter info at: (%g,%g,%g)",
				center[0], center[1], center[2]);
			return false;
		}

		w = pBSide->pVisibleHull;
		if (!w) {
			continue;
		}

		// see if we have crated this inter area portals before.
		LvlInterPortalArr::ConstIterator it = interPortals.begin();
		for (; it != interPortals.end(); ++it)
		{
			const LvlInterPortal& iap = *it;
			// same side instance?
			if (pBSide == iap.pSide)
			{
				// area match?
				if (p->nodes[0]->area == iap.area0 && p->nodes[1]->area == iap.area1) {
					break;
				}
				// what about other direction?
				if (p->nodes[1]->area == iap.area0 && p->nodes[0]->area == iap.area1) {
					break;
				}

			}
		}

		// did we find a match?
		if (it != interPortals.end()) {
			continue;
		}

		// add a new one.
		LvlInterPortal& iap = interPortals.AddOne();

		if (pBSide->planenum == p->onNode->planenum) {
			iap.area0 = p->nodes[0]->area;
			iap.area1 = p->nodes[1]->area;
		}
		else {
			iap.area0 = p->nodes[1]->area;
			iap.area1 = p->nodes[0]->area;
		}

		X_LOG0("Portal", "inter connection: ^8%i^7 <-> ^8%i",
			iap.area0, iap.area1);
		iap.pSide = pBSide;
	}
	return true;
}

bool LvlEntity::MakeStructuralFaceList(void)
{
	X_LOG0("LvlEnt", "MakeStructuralFaceList");
	X_ASSERT(bspFaces == nullptr, "bspFace already allocated")(bspFaces);

#if 1 // reverse toggle.
	size_t i, x;

	for (i = 0; i < brushes.size(); i++)
#else
	size_t x;
	int32_t i;

	for (i = ent.brushes.size() - 1; i >= 0; i--)
#endif
	{
		LvlBrush& brush = brushes[i];

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

			engine::MaterialFlags flags = side.matInfo.getFlags();

			// if combined flags are portal, check what this side is.
			if (brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL))
			{
				if (!flags.IsSet(engine::MaterialFlag::PORTAL))
				{
					continue;
				}
			}

			bspFace* pFace = X_NEW(bspFace, g_arena, "BspFace");
			pFace->planenum = side.planenum & ~1;
			pFace->w = side.pWinding->Copy(g_arena);
			pFace->pNext = bspFaces;

			if (flags.IsSet(engine::MaterialFlag::PORTAL)) {
				pFace->portal = true;
			}

			bspFaces = pFace;
		}
	}

	return true;
}


bool LvlEntity::FacesToBSP(XPlaneSet& planeSet)
{
	X_LOG0("LvlEntity", "Building face list");

	if (!this->bspFaces) {
		X_LOG0("LvlEntity", "Face list empty.");
		return false;
	}

	struct bspTree& root = this->bspTree;
	root.bounds.clear();
	root.headnode = X_NEW(bspNode, g_arena, "BspNode");

	size_t numFaces = 0;

	bspFace* pFace = bspFaces;
	for (; pFace; pFace = pFace->pNext)
	{
		numFaces++;

		const bspFace& face = *pFace;
		const XWinding& winding = *face.w;

		for (int32_t i = 0; i < winding.getNumPoints(); i++)
		{
			root.bounds.add(winding[i].asVec3());
		}
	}

	X_LOG0("LvlEntity", "num faces: ^8%i", numFaces);

	// copy bounds.
	root.headnode->bounds = root.bounds;

	FaceTreeBuilder treeBuilder(planeSet);

	if (!treeBuilder.Build(root.headnode, bspFaces)) {
		X_LOG0("LvlEntity", "failed to build tree");
		return false;
	}

	// the have all been deleted
	bspFaces = nullptr;

	X_LOG0("LvlEntity", "num leafs: ^8%i", treeBuilder.getNumLeafs());
	return true;
}

bool LvlEntity::MakeTreePortals(XPlaneSet& planeSet)
{
	return bspPortal::MakeTreePortals(planeSet,this);
}

bool LvlEntity::FilterBrushesIntoTree(XPlaneSet& planeSet)
{
	size_t		numUnique, numClusters, r;
	LvlBrush	*b, *newb;

	X_LOG0("LvlEntity", "^5----- FilterBrushesIntoTree -----");

	numUnique = 0;
	numClusters = 0;
	r = 0;

	LvlEntity::LvlBrushArr::Iterator it = brushes.begin();
	for (; it != brushes.end(); ++it)
	{
		numUnique++;

		b = it;

		newb = X_NEW(LvlBrush, g_arena, "BrushCopy")(*b);

		r = newb->FilterBrushIntoTree_r(planeSet, bspTree.headnode);

		numClusters += r;
	}

	X_LOG0("LvlEntity", "^8%5i^7 total brushes", numUnique);
	X_LOG0("LvlEntity", "^8%5i^7 cluster references", numClusters);
	return true;
}


bool LvlEntity::FloodEntities(XPlaneSet& planeSet, LvlEntsArr& ents, 
	mapfile::XMapFile* pMap)
{
	X_LOG0("LvlEntity", "^5----- FloodEntities -----");

	struct bspTree* tree;
	bspNode* headnode;
	bool inside;
	int32_t i;
	size_t floodedLeafs;

	tree = &bspTree;
	headnode = tree->headnode;
	inside = false;
	floodedLeafs = 0;

	// not occupied yet.
	tree->outside_node.occupied = 0;

	// iterate the map ents.
	for (i = 1; i < pMap->getNumEntities(); i++)
	{
		mapfile::XMapEntity* mapEnt = pMap->getEntity(i);
		LvlEntity& lvlEnt = ents[i];

		mapfile::XMapEntity::PairIt it = mapEnt->epairs.find("origin");
		if (it == mapEnt->epairs.end()){
			continue;
		}

		if (lvlEnt.PlaceOccupant(planeSet, headnode, floodedLeafs)) {
			inside = true;
		}

		// check if the outside nodes has been occupied.
		if (tree->outside_node.occupied)
		{
			X_ERROR("LvlEntity", "Leak detected!");
			X_ERROR("LvlEntity", "Entity: %i", i);
			X_ERROR("LvlEntity", "origin: %g %g %g",
				lvlEnt.origin.x,
				lvlEnt.origin.y,
				lvlEnt.origin.z);

		}
	}

	X_LOG0("LvlEntity", "^8%5i^7 flooded leafs", floodedLeafs);

	if (!inside)
	{
		X_ERROR("LvlEntity", "no entities in open -- no filling");
	}
	else if (tree->outside_node.occupied)
	{
		X_ERROR("LvlEntity", "entity reached from outside -- no filling");
	}

	return (bool)(inside && !tree->outside_node.occupied);
}

bool LvlEntity::PlaceOccupant(XPlaneSet& planeSet, bspNode* pHeadNode,
	size_t& floodedNum)
{
	X_ASSERT_NOT_NULL(pHeadNode);
	bspNode* node;
	float	d;

	// find the leaf to start in
	node = pHeadNode;
	while (node->planenum != PLANENUM_LEAF)
	{
		const Planef& plane = planeSet[node->planenum];
		d = plane.distance(origin);
		if (d >= 0.0f) {
			node = node->children[0];
		}
		else {
			node = node->children[1];
		}
	}

	if (node->opaque) {
		return false;
	}

	node->FloodPortals_r(1, floodedNum);
	return true;
}

bool LvlEntity::FillOutside(void)
{
	FillStats stats;

	X_LOG0("LvlEntity", "^5----- FillOutside -----");
	bspTree.headnode->FillOutside_r(stats);

	stats.print();
	return true;
}

bool LvlEntity::ClipSidesByTree(XPlaneSet& planeSet)
{
	X_LOG0("LvlEntity", "^5----- ClipSidesByTree -----");

	size_t i, x;

	for (i = 0; i < brushes.size(); i++)
	{
		LvlBrush& brush = brushes[i];

		for (x = 0; x < brush.sides.size(); x++)
		{
			LvlBrushSide& side = brush.sides[x];

			if (!side.pWinding) {
				continue;
			}

			side.pVisibleHull = nullptr;

			XWinding* w = side.pWinding->Copy(g_arena);

			bspTree.headnode->ClipSideByTree_r(planeSet, w, side);
		}
	}

	return true;
}

bool LvlEntity::FloodAreas(void)
{
	X_LOG0("LvlEntity", "^5----- FloodAreas -----");

	numAreas = 0;
	// find how many we have.
	bspTree.headnode->FindAreas_r(numAreas);

	X_LOG0("LvlEntity", "^8%5i^7 areas", numAreas);


	// check we not missed.
	if (!bspTree.headnode->CheckAreas_r())
		return false;

	// skip inter area portals if only one?
	if (numAreas < 2) {
		X_LOG0("LvlEntity", "Skipping inter portals. less than two area's");
		return true;
	}

	// we want to create inter area portals now.
	if (!FindInterAreaPortals()) {
		X_ERROR("LvlEntity", "Failed to calculate the inter area portal info.");
		return false;
	}

	return true;
}


bool LvlEntity::PruneNodes(void)
{
	X_LOG0("LvlEntity", "^5----- PruneNodes -----");
	
	if (!bspTree.headnode) {
		X_ERROR("LvlEntity", "Failed to prineNodes the tree is invalid.");
		return false;
	}

	int32_t prePrune = bspTree.headnode->NumChildNodes();

	bspTree.headnode->PruneNodes_r();

	int32_t postPrune = bspTree.headnode->NumChildNodes();
	int32_t numNodes = bspNode::NumberNodes_r(bspTree.headnode, 0);

#if X_DEBUG
	X_ASSERT(numNodes == postPrune, "Invalid node couts. prunt and num don't match")(numNodes, postPrune);
#endif

	X_LOG0("LvlEntity", "prePrune: ^8%i", prePrune);
	X_LOG0("LvlEntity", "postPrune: ^8%i", postPrune);
	X_LOG0("LvlEntity", "numNodes: ^8%i", numNodes);
	return true;
}


bool IsPointInAnyArea(XPlaneSet& planeSet, const Vec3f& pos, int32_t& areaOut, bspNode* pNode)
{
	X_ASSERT_NOT_NULL(pNode);
	bspNode* pCurNode = pNode;

	while (1)
	{
		const Planef& plane = planeSet[pCurNode->planenum];
		float dis = plane.distance(pos);

		if (dis > 0.f) {
			pCurNode = pCurNode->children[0];
		}
		else {
			pCurNode = pCurNode->children[1];
		}

		if (pCurNode->IsSolidLeaf()) {
			areaOut = -1; // in solid
			return false;
		}

		if (pCurNode->IsAreaLeaf())
		{
			areaOut = pCurNode->area;
			X_LOG0("Area", "Point (%g,%g,%g) is in area: %i",
				pos.x, pos.y, pos.z, pCurNode->area);
			return true;
		}
	}

	areaOut = -1;
	return false;
}

size_t AreaForOrigin_r(XPlaneSet& planeSet, const Sphere& sphere, const AABB& bounds, bspNode* pNode)
{
	X_ASSERT_NOT_NULL(pNode);
	bspNode* pCurNode = pNode;
	size_t numAreas = 0;

	do
	{
		if (pCurNode->IsAreaLeaf())
		{
			const AABB& b = pCurNode->bounds;
			
		//	if (b.containsBox(bounds))
			{
				X_LOG0("Test", "Area: %i (^6%g,%g,%g^7) <=> (^6%g,%g,%g^7)", 
					pCurNode->area,
					b.min[0], b.min[1], b.min[2],
					b.max[0], b.max[1], b.max[2]);
			}
			return 1;
		}

		const Planef& plane = planeSet[pCurNode->planenum];

		float sd = plane.distance(sphere.center());

		if (sd >= sphere.radius()) 
		{
			pCurNode = pCurNode->children[0];
			if (!pCurNode->IsSolidLeaf()) {	// 0 = solid
				AreaForOrigin_r(planeSet, sphere, bounds, pCurNode);
			}
			return 0;
		}
		if (sd <= -sphere.radius()) 
		{
			pCurNode = pCurNode->children[1];
			if (!pCurNode->IsSolidLeaf()) {	// 0s = solid
				AreaForOrigin_r(planeSet, sphere, bounds, pCurNode);
			}
			return 0 ;
		}

		Vec3f boundsVecs[2];
		boundsVecs[0] = bounds.min;
		boundsVecs[1] = bounds.max;

		Vec3f points[8];
		size_t i;
		for (i = 0; i < 8; i++)
		{
			points[i][0] = boundsVecs[i & 1][0];
			points[i][1] = boundsVecs[(i >> 1) & 1][1];
			points[i][2] = boundsVecs[(i >> 2) & 1][2];
		}

		bool front = false;
		bool back = false;
		for (i = 0; i < 8; i++)
		{
			float d;

			d = points[i] * plane.getNormal() + plane.getDistance();
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
			pCurNode = pCurNode->children[0];
			if (!pCurNode->IsSolidLeaf()) {	// 0 = solid
				AreaForOrigin_r(planeSet, sphere, bounds, pCurNode);
			}
		}
		if (back) {
			pCurNode = pCurNode->children[1];
			if (!pCurNode->IsSolidLeaf()) {	// 0 = solid
				AreaForOrigin_r(planeSet, sphere, bounds, pCurNode);
			}
		}

		/*
		const Planef& plane = planeSet[pCurNode->planenum];
		PlaneSide::Enum side = bounds.planeSide(plane);

		if (side == PlaneSide::FRONT) {
			pCurNode = pCurNode->children[0];
		}
		else if (side == PlaneSide::BACK) {
			pCurNode = pCurNode->children[1];
		}
		else
		{
			// travel down both paths.
			if (!pCurNode->children[1]->IsSolidLeaf()) {
				numAreas += AreaForOrigin_r(planeSet, bounds, pCurNode->children[1]);
			}

			pCurNode = pCurNode->children[0];
		}
		*/
	} while (!pCurNode->IsSolidLeaf());

	return numAreas;
}


bool LvlEntity::PutEntsInAreas(XPlaneSet& planeSet, core::Array<LvlEntity>& ents,
	mapfile::XMapFile* pMap)
{
	X_ASSERT_NOT_NULL(pMap);
	int32_t i;

	LvlEntity& world = ents[0];

	// iterate the map ents.
	for (i = 1; i < pMap->getNumEntities(); i++)
	{
		mapfile::XMapEntity* mapEnt = pMap->getEntity(i);
		LvlEntity& lvlEnt = ents[i];

		// for now just add the static models ents to world ent.
		{
			mapfile::XMapEntity::PairIt it = mapEnt->epairs.find("classname");
			if (it == mapEnt->epairs.end()) {
				continue;
			}
			const core::string& className = it->second;
			if (className != X_CONST_STRING("misc_model")) {
				continue;
			}

			it = mapEnt->epairs.find("model");
			if (it == mapEnt->epairs.end()) {
				X_WARNING("Entity", "misc_model missing 'model' kvp at: (^8%g,%g,%g^7)",
					lvlEnt.origin[0], lvlEnt.origin[1], lvlEnt.origin[2]);
				continue;
			}

			const core::string& modelName = it->second;
			X_LOG0("Entity", "Ent model: \"%s\"", modelName.c_str());
		}

		// we want to find out what areas the bounds of this model
		// are in, then add a refrence for that model to both areas.
		// Since we will need to draw the model if either of the area's are active.
		// i will want to store the model in both of the area's indivudual lists.
		// at runtime I will use frame id's to work out what has already been drawn each frame.


		// this dose mean i need to know the bounds of the model.
		// meaning i must load it.

		X_LOG0("Entity", "Finding areas for ent: %i origin: (^8%g,%g,%g^7)", i,
			lvlEnt.origin.x, lvlEnt.origin.y, lvlEnt.origin.z);

		AABB bounds;
		bounds.set(lvlEnt.bounds.min + lvlEnt.origin,
			lvlEnt.bounds.max + lvlEnt.origin);

		// make a sphere.
		Sphere sphere(bounds);

		size_t numAreas = AreaForOrigin_r(planeSet, sphere, bounds, bspTree.headnode);

		if (numAreas < 1)
		{
			X_ERROR("Entity", "ent resides in zero areas.");
		}
	}
	return true;
}

