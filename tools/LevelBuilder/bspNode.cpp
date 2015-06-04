#include "stdafx.h"
#include "BSPTypes.h"
#include "LvlTypes.h"



void bspNode::MakeTreePortals_r(XPlaneSet& planeSet)
{
	X_ASSERT_NOT_NULL(this);
	size_t i;

	CalcNodeBounds();

	if (bounds.isEmpty()) {
		X_WARNING("Portal", "node without a volume");
	}

	for (i = 0; i < 3; i++)
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
	this->SplitPortals(planeSet);

	children[0]->MakeTreePortals_r(planeSet);
	children[1]->MakeTreePortals_r(planeSet);
}


void bspNode::CalcNodeBounds(void)
{
	bspPortal* p;
	int	s, i;

	// calc mins/maxs for both leafs and nodes
	bounds.clear();
	for (p = portals; p; p = p->next[s]) 
	{
		s = (p->nodes[1] == this);

		for (i = 0; i < p->pWinding->GetNumPoints(); i++)
		{
			const Vec5f& point = (*p->pWinding)[i];
			bounds.add(point.asVec3());
		}
	}
}

XWinding* bspNode::GetBaseWinding(XPlaneSet& planeSet)
{
	XWinding*	w;
	bspNode	*	n;

	w = X_NEW(XWinding, g_arena, "WindingForNode")(planeSet[planenum]);

	// clip by all the parents
	bspNode* pNode = this;

	for (n = pNode->parent; n && w;)
	{
		Planef &plane = planeSet[n->planenum];

		if (n->children[0] == pNode) 
		{
			// take front
			w = w->Clip(plane, BASE_WINDING_EPSILON);
		}
		else 
		{
			// take back
			Planef	back = -plane;
			w = w->Clip(back, BASE_WINDING_EPSILON);
		}
		pNode = n;
		n = n->parent;
	}

	return w;

}

void bspNode::FloodPortals_r(int32_t dist, size_t& floodedNum)
{
	bspPortal* p;
	int32_t s;

	if (occupied) {
		return;
	}
	if (opaque) {
		return;
	}

	floodedNum++;

	occupied = dist;

	for (p = portals; p; p = p->next[s]) 
	{
		s = (p->nodes[1] == this);
		p->nodes[!s]->FloodPortals_r(dist + 1, floodedNum);
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
	f = children[0];
	b = children[1];

	for (p = portals; p; p = next_portal)
	{
		// a portal is linked to two nodes.
		// a node can have many portals.
		// if we are to the left of the portal.
		// the node is on the right.
		if (p->nodes[0] == this) {
			side = 0;
		}
		else if (p->nodes[1] == this) {
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
		p->RemoveFromNode( p->nodes[0]);
		p->RemoveFromNode(p->nodes[1]);

		// cut the portal into two portals, one on each side of the cut plane
		// this means that the 
		p->pWinding->Split(*plane, SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

		if (frontwinding && frontwinding->IsTiny())
		{
			X_DELETE_AND_NULL(frontwinding, g_arena);
		}

		if (backwinding && backwinding->IsTiny())
		{
			X_DELETE_AND_NULL(backwinding, g_arena);
		}

		if (!frontwinding && !backwinding)
		{
			// tiny windings on both sides
			continue;
		}


		if (!frontwinding)
		{
			X_DELETE_AND_NULL(backwinding, g_arena);
			if (side == 0)
				p->AddToNodes(b, other_node);
			else
				p->AddToNodes(other_node, b);
			continue;
		}
		if (!backwinding)
		{
			X_DELETE_AND_NULL(frontwinding, g_arena);
			if (side == 0)
				p->AddToNodes(f, other_node);
			else
				p->AddToNodes(other_node, f);
			continue;
		}

		// the winding is split
		// means the protal span across the current binary tree node more than the elipson.
		// so make a new portal
		new_portal = X_NEW(bspPortal, g_arena, "Portal");
		*new_portal = *p;
		new_portal->pWinding = backwinding;
		// delete the portals old winding and assing the new one.
		X_DELETE(p->pWinding, g_arena);
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
		children[0]->FillOutside_r(stats);
		children[1]->FillOutside_r(stats);
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
	XWinding *front, *back;

	if (!w) {
		return;
	}

	if (planenum != PLANENUM_LEAF)
	{
		if (side.planenum == planenum) {
			children[0]->ClipSideByTree_r(planes, w, side);
			return;
		}
		if (side.planenum == (planenum ^ 1)) {
			children[1]->ClipSideByTree_r(planes, w, side);
			return;
		}

		w->Split(planes[planenum], ON_EPSILON, &front, &back);

		X_DELETE(w, g_arena);

		children[0]->ClipSideByTree_r(planes, front, side);
		children[1]->ClipSideByTree_r(planes, back, side);

		return;
	}

	// if opaque leaf, don't add
	if (!opaque)
	{
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

void bspNode::FindAreas_r(size_t& numAreas)
{
	if (planenum != PLANENUM_LEAF) {
		children[0]->FindAreas_r(numAreas);
		children[1]->FindAreas_r(numAreas);
		return;
	}

	if (opaque) {
		return;
	}

	if (area != -1) {
		return;	// allready got it
	}

	size_t areaFloods = 0;
	this->FloodAreas_r(numAreas, areaFloods);

	X_LOG0("Lvl", "area %i has %i leafs", numAreas, areaFloods);
	numAreas++;

}

void bspNode::FloodAreas_r(size_t area, size_t& areaFloods)
{
	bspPortal* p;
	bspNode* other;
	int	s;

	if (area != -1) {
		return;	// allready got it
	}
	if (opaque) {
		return;
	}

	areaFloods++;
	area = area;

	for (p = portals; p; p = p->next[s])
	{
		s = (p->nodes[1] == this);
		other = p->nodes[!s];

		if (!p->PortalPassable()) {
			continue;
		}

		// can't flood through an area portal
		if (p->HasAreaPortalSide()) {
			continue;
		}

		other->FloodAreas_r(area, areaFloods);
	}
}

bool bspNode::CheckAreas_r(void)
{
	if (planenum != PLANENUM_LEAF)
	{
		if (!children[0]->CheckAreas_r())
			return false;
		if (!children[0]->CheckAreas_r())
			return false;

		return true;
	}

	if (!opaque && area < 0) {
		X_ERROR("bspNode", "node has a invalid area: %i", area);
		return false;
	}

	return true;
}