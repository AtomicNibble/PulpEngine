#include "stdafx.h"
#include "BSPTypes.h"
#include "LvlBrush.h"

X_NAMESPACE_BEGIN(lvl)

void bspNode::MakeTreePortals_r(XPlaneSet& planeSet)
{
	X_ASSERT_NOT_NULL(this);

	CalcNodeBounds();

	if (bounds.isEmpty()) {
		X_WARNING("Portal", "node without a volume");
	}

	for (int32_t i = 0; i < 3; i++)
	{
		if (bounds.min[i] < level::MIN_WORLD_COORD ||
			bounds.max[i] > level::MAX_WORLD_COORD) 
		{
			X_WARNING("Portal", "node with unbounded volume");
			break;
		}
	}
	if (planenum == PLANENUM_LEAF) {
		return;
	}

	bspPortal::MakeNodePortal(planeSet, this);
	SplitPortals(planeSet);

	children[Side::FRONT]->MakeTreePortals_r(planeSet);
	children[Side::BACK]->MakeTreePortals_r(planeSet);
}


void bspNode::CalcNodeBounds(void)
{
	bounds.clear();
	size_t s;

	// calc mins/maxs for both leafs and nodes
	for (auto* p = portals; p; p = p->next[s]) 
	{
		s = (p->nodes[Side::BACK] == this);

		for (size_t i = 0; i < p->pWinding->getNumPoints(); i++)
		{
			const Vec5f& point = (*p->pWinding)[i];
			bounds.add(point.asVec3());
		}
	}
}

core::UniquePointer<XWinding> bspNode::getBaseWinding(XPlaneSet& planeSet)
{
	auto w = core::makeUnique<XWinding>(g_windingArena, planeSet[planenum]);
	
	// clip by all the parents
	bspNode* pNode = this;
	for (auto* n = pNode->parent; n && w;)
	{
		Planef &plane = planeSet[n->planenum];

		if (n->children[Side::FRONT] == pNode) 
		{
			// take front
			if (!w->clip(plane, BASE_WINDING_EPSILON)) {
				w.reset();
			}
		}
		else 
		{
			// take back
			Planef	back = -plane;
			if(!w->clip(back, BASE_WINDING_EPSILON)) {
				w.reset();
			}
		}
		pNode = n;
		n = n->parent;
	}

	return w;
}

void bspNode::FloodPortals_r(int32_t dist, size_t& floodedNum)
{
	if (occupied || opaque) {
		return;
	}
	
	floodedNum++;

	occupied = dist;

	int32_t side;
	for (auto* p = portals; p; p = p->next[side]) 
	{
		side = (p->nodes[Side::BACK] == this);
		p->nodes[!side]->FloodPortals_r(dist + 1, floodedNum);
	}
}

void bspNode::SplitPortals(XPlaneSet& planes)
{
	bspPortal	*p, *next_portal, *new_portal;
	bspNode		*f, *b, *other_node;
	int			side;
	Planef		*plane;
	XWinding	*frontwinding, *backwinding;

	// get the plane for this node.
	plane = &planes[planenum];
	// front and back nodes of this node that we are going to split.
	f = children[Side::FRONT];
	b = children[Side::BACK];

	for (p = portals; p; p = next_portal)
	{
		// a portal is linked to two nodes.
		// a node can have many portals.
		// if we are to the left of the portal.
		// the node is on the right.
		if (p->nodes[Side::FRONT] == this) {
			side = 0;
		}
		else if (p->nodes[Side::BACK] == this) {
			side = 1;
		}
		else {
			X_ERROR("bspNode", "SplitPortals: mislinked portal");
			side = 0;	// quiet a compiler warning
		}

		// the next portal, on the same side.
		next_portal = p->next[side];
		// this is the node on the opposite size.
		other_node = p->nodes[!side];

		// remove the portals from the nodes linked.
		// list ready for when we added the new split nodes.
		p->RemoveFromNode(p->nodes[Side::FRONT]);
		p->RemoveFromNode(p->nodes[Side::BACK]);

		// cut the portal into two portals, one on each side of the cut plane
		// this means that the 
		p->pWinding->Split(*plane, SPLIT_WINDING_EPSILON, 
			&frontwinding, &backwinding, g_windingArena);

		if (frontwinding && frontwinding->isTiny())
		{
			X_DELETE_AND_NULL(frontwinding, g_windingArena);
		}

		if (backwinding && backwinding->isTiny())
		{
			X_DELETE_AND_NULL(backwinding, g_windingArena);
		}

		if (!frontwinding && !backwinding)
		{
			// tiny windings on both sides
			continue;
		}


		if (!frontwinding)
		{
			X_DELETE_AND_NULL(backwinding, g_windingArena);

			if (side == 0) {
				p->AddToNodes(b, other_node);
			}
			else {
				p->AddToNodes(other_node, b);
			}

			continue;
		}
		if (!backwinding)
		{
			X_DELETE_AND_NULL(frontwinding, g_windingArena);

			if (side == 0) {
				p->AddToNodes(f, other_node);
			}
			else {
				p->AddToNodes(other_node, f);
			}

			continue;
		}

		// the winding is split
		// means the protal span across the current binary tree node more than the elipson.
		// so make a new portal
		new_portal = X_NEW(bspPortal, g_arena, "Portal");
		*new_portal = *p;
		new_portal->pWinding = backwinding;
		// delete the portals old winding and assing the new one.
		X_DELETE(p->pWinding, g_windingArena);
		p->pWinding = frontwinding;

		if (side == 0)
		{
			p->AddToNodes(f, other_node);
			new_portal->AddToNodes(b, other_node);
		}
		else
		{
			p->AddToNodes(other_node, f);
			new_portal->AddToNodes(other_node, b);
		}
	}

	portals = nullptr;
}

void bspNode::FillOutside_r(FillStats& stats)
{
	if (planenum != PLANENUM_LEAF)
	{
		children[Side::FRONT]->FillOutside_r(stats);
		children[Side::BACK]->FillOutside_r(stats);
		return;
	}

	// is this node occupied?
	if (!occupied)
	{
		// if the node is not opaque it must be outside, otherwise it's a solid.
		if (!opaque) {
			stats.numOutside++;
			opaque = true;
		}
		else {
			stats.numSolid++;
		}
	}
	else {
		stats.numInside++;
	}
}

void bspNode::ClipSideByTree_r(XPlaneSet& planes, XWinding* w, LvlBrushSide& side)
{

	if (!w) {
		return;
	}

	if (planenum != PLANENUM_LEAF)
	{
		if (side.planenum == planenum) {
			children[Side::FRONT]->ClipSideByTree_r(planes, w, side);
			return;
		}
		if (side.planenum == (planenum ^ 1)) {
			children[Side::BACK]->ClipSideByTree_r(planes, w, side);
			return;
		}

		XWinding *pFront, *pBack;

		w->SplitMove(planes[planenum], ON_EPSILON, &pFront, &pBack, g_windingArena);

		X_DELETE(w, g_windingArena);

		children[Side::FRONT]->ClipSideByTree_r(planes, pFront, side);
		children[Side::BACK]->ClipSideByTree_r(planes, pBack, side);
		return;
	}

	// if opaque leaf, don't add
	if (!opaque)
	{
		if (!side.pVisibleHull) {
			side.pVisibleHull = w->Move(g_windingArena);
		}
		else {
			side.pVisibleHull->AddToConvexHull(w, planes[side.planenum].getNormal());
		}
	}

	X_DELETE(w, g_windingArena);
	return;
}

int32_t bspNode::CheckWindingInAreas_r(XPlaneSet& planes, const XWinding* w)
{
	if (!w) {
		return -1;
	}

	if (planenum != PLANENUM_LEAF)
	{
		XWinding* pFront, *pBack;
		w->Split(planes[planenum], ON_EPSILON, &pFront, &pBack, g_windingArena);

		int32_t a1 = children[Side::FRONT]->CheckWindingInAreas_r(planes, pFront);
		int32_t a2 = children[Side::BACK]->CheckWindingInAreas_r(planes, pBack);
		
		X_DELETE(pFront, g_windingArena);
		X_DELETE(pBack, g_windingArena);

		if (a1 == -2 || a2 == -2) {
			return -2;	// different
		}
		if (a1 == -1) {
			return a2;	// one solid
		}
		if (a2 == -1) {
			return a1;	// one solid
		}

		if (a1 != a2) {
			return -2;	// cross areas
		}
		return a1;
	}

	return area;
}

void bspNode::FindAreas_r(size_t& numAreas)
{
	if (planenum != PLANENUM_LEAF) {
		children[Side::FRONT]->FindAreas_r(numAreas);
		children[Side::BACK]->FindAreas_r(numAreas);
		return;
	}

	if (opaque) {
		return;
	}

	if (area != -1) {
		return;	// allready got it
	}

	size_t areaFloods = 0;
	FloodAreas_r(numAreas, areaFloods);

	X_LOG1("Lvl", "area ^8%" PRIuS "^7 has ^8%" PRIuS "^7 leafs", numAreas, areaFloods);
	numAreas++;
}

void bspNode::FloodAreas_r(size_t areaNum, size_t& areaFloods)
{
	if (area != -1) {
		return;	// allready got it
	}
	if (opaque) {
		return;
	}

	areaFloods++;
	area = safe_static_cast<int32_t,size_t>(areaNum);

	int32_t side;
	for (auto* p = portals; p; p = p->next[side])
	{
		side = (p->nodes[Side::BACK] == this);
		auto* pOther = p->nodes[!side];

		if (!p->PortalPassable()) {
			continue;
		}

		// can't flood through an area portal
		if (p->HasAreaPortalSide()) {
			continue;
		}

		pOther->FloodAreas_r(areaNum, areaFloods);
	}
}

bool bspNode::CheckAreas_r(void)
{
	if (planenum != PLANENUM_LEAF)
	{
		if (!children[Side::FRONT]->CheckAreas_r()) {
			return false;
		}
		if (!children[Side::BACK]->CheckAreas_r()) {
			return false;
		}

		return true;
	}

	if (!opaque && area < 0) {
		X_ERROR("bspNode", "node has a invalid area: %i", area);
		return false;
	}

	return true;
}


int32_t	bspNode::PruneNodes_r(void)
{
	int32_t	a1, a2;

	if (planenum == PLANENUM_LEAF) {
		return area;
	}

	a1 = children[Side::FRONT]->PruneNodes_r();
	a2 = children[Side::BACK]->PruneNodes_r();

	if (a1 != a2 || a1 == PLANENUM_AREA_DIFF) {
		return PLANENUM_AREA_DIFF;
	}

	// free all the nodes below this point
	children[Side::FRONT]->FreeTreePortals_r();
	children[Side::FRONT]->FreeTree_r();
	children[Side::BACK]->FreeTreePortals_r();
	children[Side::BACK]->FreeTree_r();

	core::zero_object(children);

	// change this node to a leaf
	planenum = PLANENUM_LEAF;
	area = a1;

	return a1;
}

int32_t bspNode::NumChildNodes(void)
{
	return NumChildNodes_r(this);
}

void bspNode::FreeTreePortals_r(void)
{
	// free all the portals.

	// free children
	if (planenum != PLANENUM_LEAF)
	{
		children[Side::FRONT]->FreeTreePortals_r();
		children[Side::BACK]->FreeTreePortals_r();
	}

	// free portals
	bspPortal* nextp;
	int32_t side;
	for (auto* p = portals; p; p = nextp)
	{
		side = (p->nodes[1] == this);
		nextp = p->next[side];

		p->RemoveFromNode(p->nodes[!side]);
		X_DELETE(p, g_arena);
	}
	
	portals = nullptr;
}

void bspNode::FreeTree_r(void)
{
	// free all the sub nodes and self.
	// free children
	if (planenum != PLANENUM_LEAF)
	{
		children[Side::FRONT]->FreeTree_r();
		children[Side::BACK]->FreeTree_r();
	}

	X_DELETE(this, g_bspNodeArena);
}

void bspNode::WriteNodes_r(XPlaneSet& planes, core::XFile* pFile)
{
	int32_t childIds[2];
	size_t i;

	if (planenum == PLANENUM_LEAF) {
		X_WARNING("bspNode", "Got a leaf plane while writing nodes.");
		return;
	}

	for (i = 0; i < 2; i++)
	{
		if (children[i]->planenum == PLANENUM_LEAF) {
			childIds[i] = -1 - children[i]->area; // leafs with area -1 get child of 0
		}
		else {
			childIds[i] = children[i]->nodeNumber;
		}
	}

	// get the plane.
	const Planef& plane = planes[planenum];

	pFile->writeObj(plane);
	pFile->writeObj(childIds);

	// process the children, if they are not leafs.
	if (childIds[Side::FRONT] > 0) {
		children[Side::FRONT]->WriteNodes_r(planes, pFile);
	}
	if (childIds[Side::BACK] > 0) {
		children[Side::BACK]->WriteNodes_r(planes, pFile);
	}
}

int32_t bspNode::NumberNodes_r(bspNode* pNode, int32_t nextNumber)
{
	X_ASSERT_NOT_NULL(pNode);

	if (pNode->planenum == PLANENUM_LEAF) {
		return nextNumber;
	}

	pNode->nodeNumber = nextNumber;
	nextNumber++;
	nextNumber = NumberNodes_r(pNode->children[Side::FRONT], nextNumber);
	nextNumber = NumberNodes_r(pNode->children[Side::BACK], nextNumber);

	return nextNumber;
}


int32_t bspNode::NumChildNodes_r(bspNode* pNode)
{
	X_ASSERT_NOT_NULL(pNode);
	// leaf don't count.
	if (pNode->planenum == PLANENUM_LEAF) {
		return 0;
	}

	int32_t num = 1;

	if (pNode->children[Side::FRONT]) {
		num += NumChildNodes_r(pNode->children[Side::FRONT]);
	}
	if (pNode->children[Side::BACK]) {
		num += NumChildNodes_r(pNode->children[Side::BACK]);
	}

	return num;
}


X_NAMESPACE_END