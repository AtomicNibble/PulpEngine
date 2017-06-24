#include "stdafx.h"
#include "BSPTypes.h"
#include "LvlTypes.h"

#include "FaceTreeBuilder.h"

#include "LvlFmts/mapFile/MapTypes.h"
#include "LvlFmts/mapFile/MapFile.h"

// ===================

X_NAMESPACE_BEGIN(lvl)

bool LvlEntity::FindInterAreaPortals(void)
{
	if (!bspTree_.headnode) {
		X_ERROR("LvlEnt", "Can't find inter area portal information. tree is invalid.");
		return false;
	}

	return FindInterAreaPortals_r(bspTree_.headnode);
}

bool LvlEntity::FindInterAreaPortals_r(bspNode* node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		if (!FindInterAreaPortals_r(node->children[0])) {
			return false;
		}
		if (!FindInterAreaPortals_r(node->children[1])) {
			return false;
		}

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

		X_LOG1("Portal", "inter connection: ^8%" PRIi32 "^7 <-> ^8%" PRIi32, iap.area0, iap.area1);

		iap.pSide = pBSide;
	}
	return true;
}

bool LvlEntity::MakeStructuralFaceList(void)
{
	X_LOG0("LvlEnt", "MakeStructuralFaceList");
	X_ASSERT(pBspFaces == nullptr, "bspFace already allocated")(pBspFaces);

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

			bspFace* pFace = X_NEW(bspFace, g_bspFaceArena, "BspFace");
			pFace->planenum = side.planenum & ~1;
			pFace->w = side.pWinding->Copy(g_arena);
			pFace->pNext = pBspFaces;

			if (flags.IsSet(engine::MaterialFlag::PORTAL)) {
				pFace->portal = true;
			}

			pBspFaces = pFace;
		}
	}

	return true;
}


bool LvlEntity::FacesToBSP(XPlaneSet& planeSet)
{
	X_LOG0("LvlEntity", "Building face list");

	if (!this->pBspFaces) {
		X_ERROR("LvlEntity", "Face list empty.");
		return false;
	}

	bspTree& root = bspTree_;
	root.bounds.clear();
	root.headnode = X_NEW(bspNode, g_bspNodeArena, "BspNode");

	size_t numFaces = 0;

	for (bspFace* pFace = pBspFaces; pFace; pFace = pFace->pNext)
	{
		numFaces++;

		const bspFace& face = *pFace;
		const XWinding& winding = *face.w;

		for (size_t i = 0; i < winding.getNumPoints(); i++)
		{
			root.bounds.add(winding[i].asVec3());
		}
	}

	X_LOG0("LvlEntity", "num faces: ^8%" PRIuS, numFaces);

	// copy bounds.
	root.headnode->bounds = root.bounds;

	FaceTreeBuilder treeBuilder(planeSet);

	if (!treeBuilder.Build(root.headnode, pBspFaces)) {
		X_LOG0("LvlEntity", "failed to build tree");
		return false;
	}

	// the have all been deleted
	pBspFaces = nullptr;

	X_LOG0("LvlEntity", "num leafs: ^8%" PRIuS, treeBuilder.getNumLeafs());
	return true;
}

bool LvlEntity::MakeTreePortals(XPlaneSet& planeSet)
{
	return bspPortal::MakeTreePortals(planeSet,this);
}

bool LvlEntity::FilterBrushesIntoTree(XPlaneSet& planeSet)
{
	size_t		numClusters, r;
	LvlBrush	*b, *newb;

	X_LOG0("LvlEntity", "^5----- FilterBrushesIntoTree -----");

	numClusters = 0;
	r = 0;

	LvlEntity::LvlBrushArr::Iterator it = brushes.begin();
	for (; it != brushes.end(); ++it)
	{
		b = it;

		newb = X_NEW(LvlBrush, g_arena, "BrushCopy")(*b);

		r = newb->FilterBrushIntoTree_r(planeSet, bspTree_.headnode);

		numClusters += r;
	}

	X_LOG0("LvlEntity", "^8%5i^7 total brushes", brushes.size());
	X_LOG0("LvlEntity", "^8%5i^7 cluster references", numClusters);
	return true;
}


bool LvlEntity::FloodEntities(XPlaneSet& planeSet, LvlEntsArr& ents, 
	mapFile::XMapFile* pMap)
{
	X_LOG0("LvlEntity", "^5----- FloodEntities -----");

	bspTree* tree = &bspTree_;
	bspNode* headnode = tree->headnode;
	bool inside = false;
	size_t floodedLeafs = 0;

	// not occupied yet.
	tree->outside_node.occupied = 0;

	// iterate the map ents.
	for (size_t i = 1; i < pMap->getNumEntities(); i++)
	{
		mapFile::XMapEntity* mapEnt = pMap->getEntity(i);
		LvlEntity& lvlEnt = ents[i];

		auto it = mapEnt->epairs.find(X_CONST_STRING("origin"));
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
			X_ERROR("LvlEntity", "Entity: %" PRIuS, i);
			X_ERROR("LvlEntity", "origin: %g %g %g",
				lvlEnt.origin.x, lvlEnt.origin.y, lvlEnt.origin.z);

			return false;
		}
	}

	X_LOG0("LvlEntity", "^8%5" PRIuS "^7 flooded leafs", floodedLeafs);

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

	// find the leaf to start in
	bspNode* node = pHeadNode;
	while (node->planenum != PLANENUM_LEAF)
	{
		const Planef& plane = planeSet[node->planenum];
		float d = plane.distance(origin);
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
	bspTree_.headnode->FillOutside_r(stats);

	stats.print();
	return true;
}

bool LvlEntity::ClipSidesByTree(XPlaneSet& planeSet)
{
	X_LOG0("LvlEntity", "^5----- ClipSidesByTree -----");

	for (size_t i = 0; i < brushes.size(); i++)
	{
		LvlBrush& brush = brushes[i];

		for (size_t x = 0; x < brush.sides.size(); x++)
		{
			LvlBrushSide& side = brush.sides[x];

			if (!side.pWinding) {
				continue;
			}

			side.pVisibleHull = nullptr;

			XWinding* w = side.pWinding->Copy(g_arena);

			bspTree_.headnode->ClipSideByTree_r(planeSet, w, side);
		}
	}

	return true;
}

bool LvlEntity::FloodAreas(void)
{
	X_LOG0("LvlEntity", "^5----- FloodAreas -----");

	numAreas = 0;
	// find how many we have.
	bspTree_.headnode->FindAreas_r(numAreas);

	X_LOG0("LvlEntity", "^8%5" PRIuS "^7 areas", numAreas);

	// check we not missed.
	if (!bspTree_.headnode->CheckAreas_r()) {
		return false;
	}

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
	
	if (!bspTree_.headnode) {
		X_ERROR("LvlEntity", "Failed to prineNodes the tree is invalid.");
		return false;
	}

	int32_t prePrune = bspTree_.headnode->NumChildNodes();

	bspTree_.headnode->PruneNodes_r();

	int32_t postPrune = bspTree_.headnode->NumChildNodes();
	int32_t numNodes = bspNode::NumberNodes_r(bspTree_.headnode, 0);

#if X_DEBUG
	X_ASSERT(numNodes == postPrune, "Invalid node couts. prunt and num don't match")(numNodes, postPrune);
#endif

	X_LOG0("LvlEntity", "prePrune: ^8%" PRIi32, prePrune);
	X_LOG0("LvlEntity", "postPrune: ^8%" PRIi32, postPrune);
	X_LOG0("LvlEntity", "numNodes: ^8%" PRIi32, numNodes);
	return true;
}

X_NAMESPACE_END