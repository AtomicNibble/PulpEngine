#include "stdafx.h"
#include "LevelBuilder.h"

#include "MapLoader.h"

namespace
{
	#define	SIDESPACE	8

	size_t c_tinyportals = 0;
	size_t c_floodedleafs = 0;

	void AddPortalToNodes(bspPortal* p, bspNode* front, bspNode* back) 
	{
		X_ASSERT_NOT_NULL(p);

		if (p->nodes[0] || p->nodes[1]) 
		{
			X_ERROR("Portal", "Node already included");
		}

		p->nodes[0] = front;
		p->next[0] = front->portals;
		front->portals = p;

		p->nodes[1] = back;
		p->next[1] = back->portals;
		back->portals = p;
	}

	void MakeHeadnodePortals(bspTree& tree) 
	{
		AABB	bounds;
		int		i, j, n;
		Planef	bplanes[6], *pl;
		bspNode* node;
		bspPortal *p, *portals[6];


		node = tree.headnode;

		tree.outside_node.planenum = PLANENUM_LEAF;
//		tree->outside_node.brushlist = NULL;
		tree.outside_node.portals = NULL;
		tree.outside_node.opaque = false;

		// if no nodes, don't go any farther
		if (node->planenum == PLANENUM_LEAF) {
			return;
		}

		// pad with some space so there will never be null volume leafs
		for (i = 0; i<3; i++) {
			bounds.min[i] = tree.bounds.min[i] - SIDESPACE;
			bounds.max[i] = tree.bounds.max[i] + SIDESPACE;
			if (bounds.min[i] >= bounds.max[i]) {
				X_FATAL("Bsp", "Backward tree volume");
			}
		}

		for (i = 0; i<3; i++) {
			for (j = 0; j<2; j++) {
				n = j * 3 + i;

				p = X_NEW(bspPortal, g_arena, "HeadPortal");
				portals[n] = p;

				pl = &bplanes[n];
				memset(pl, 0, sizeof(*pl));
				
				if (j) {
					(*pl)[i] = -1;
					(*pl).setDistance(-bounds.max[i]);
				}
				else {
					(*pl)[i] = 1;
					(*pl).setDistance(bounds.min[i]);
				}

				p->plane = *pl;
				p->pWinding = X_NEW(XWinding, g_arena, "Winding")(*pl);

				AddPortalToNodes(p, node, &tree.outside_node);
			}
		}

		// clip the basewindings by all the other planes
		for (i = 0; i<6; i++) {
			for (j = 0; j<6; j++) {
				if (j == i) {
					continue;
				}
				portals[i]->pWinding = portals[i]->pWinding->Clip(bplanes[j], ON_EPSILON);
			}
		}

#if X_DEBUG
		X_LOG0("Portals", "Head node windings");
		// print the head nodes portal bounds.
		for (i = 0; i<6; i++) {
			portals[i]->pWinding->Print();
		}
		int break_me = 0;
#endif // !X_DEBUG
	}

	void CalcNodeBounds(bspNode* node)
	{
		bspPortal* p;
		int	s, i;

		// calc mins/maxs for both leafs and nodes
		node->bounds.clear();
		for (p = node->portals; p; p = p->next[s]) {
			s = (p->nodes[1] == node);
			for (i = 0; i < p->pWinding->GetNumPoints(); i++) 
			{
				const Vec5f& point = (*p->pWinding)[i];
				node->bounds.add(point.asVec3());
			}
		}
	}


	XWinding* BaseWindingForNode(XPlaneSet& planes, bspNode* node)
	{
		XWinding	*w;
		bspNode		*n;

		w = X_NEW(XWinding, g_arena, "WindingForNode")(planes[node->planenum]);

		// clip by all the parents
		for (n = node->parent; n && w;) {
			Planef &plane = planes[n->planenum];

			if (n->children[0] == node) {
				// take front
				w = w->Clip(plane, BASE_WINDING_EPSILON);
			}
			else {
				// take back
				Planef	back = -plane;
				w = w->Clip(back, BASE_WINDING_EPSILON);
			}
			node = n;
			n = n->parent;
		}

		return w;
	}

	void MakeNodePortal(XPlaneSet& planes, bspNode* node)
	{
		bspPortal	*new_portal, *p;
		XWinding	*w;
		Vec3f		normal;
		int			side;

		w = BaseWindingForNode(planes, node);

		// clip the portal by all the other portals in the node
		for (p = node->portals; p && w; p = p->next[side])
		{
			Planef	plane;

			if (p->nodes[0] == node)
			{
				side = 0;
				plane = p->plane;
			}
			else if (p->nodes[1] == node)
			{
				side = 1;
				plane = -p->plane;
			}
			else {
				X_ERROR("Portal", "CutNodePortals_r: mislinked portal");
				side = 0;	// quiet a compiler warning
			}

			w = w->Clip(plane, CLIP_EPSILON);
		}

		if (!w)
		{
			X_WARNING("Portal","Winding is empty");
			return;
		}

#if X_DEBUG
		w->Print();
#endif // X_DEBUG

		if (w->IsTiny())
		{
			c_tinyportals++;
			X_DELETE(w, g_arena);
			return;
		}

		new_portal = X_NEW(bspPortal, g_arena, "Portal");
		new_portal->plane = planes[node->planenum];
		new_portal->onNode = node;
		new_portal->pWinding = w;
		AddPortalToNodes(new_portal, node->children[0], node->children[1]);
	}


	void RemovePortalFromNode(bspPortal* portal, bspNode* l)
	{
		bspPortal	**pp, *t;

		// remove reference to the current portal
		pp = &l->portals;
		while (1)
		{
			t = *pp;

			if (!t) {
				X_ERROR("Portal", "RemovePortalFromNode: portal not in leaf");
			}

			if (t == portal) {
				break;
			}

			if (t->nodes[0] == l)
				pp = &t->next[0];
			else if (t->nodes[1] == l)
				pp = &t->next[1];
			else {
				X_ERROR("Portal", "RemovePortalFromNode: portal not bounding leaf");
			}
		}

		if (portal->nodes[0] == l) {
			*pp = portal->next[0];
			portal->nodes[0] = nullptr;
		}
		else if (portal->nodes[1] == l) {
			*pp = portal->next[1];
			portal->nodes[1] = nullptr;
		}
		else {
			X_ERROR("Portal", "RemovePortalFromNode: mislinked");
		}
	}


	void SplitNodePortals(XPlaneSet& planes, bspNode* node)
	{
		bspPortal	*p, *next_portal, *new_portal;
		bspNode		*f, *b, *other_node;
		int			side;
		Planef		*plane;
		XWinding	*frontwinding, *backwinding;

		// get the plane for this node.
		plane = &planes[node->planenum];
		// front and back nodes of this node that we are going to split.
		f = node->children[0];
		b = node->children[1];

		for (p = node->portals; p; p = next_portal) 
		{
			// a portal is linked to two nodes.
			// a node can have many portals.
			// if we are to the left of the portal.
			// the node is on the right.
			if (p->nodes[0] == node) {
				side = 0;
			}
			else if (p->nodes[1] == node) {
				side = 1;
			}
			else {
				X_ERROR("Portal", "SplitNodePortals: mislinked portal");
				side = 0;	// quiet a compiler warning
			}

			// the next portal, on the same side.
			next_portal = p->next[side];
			// this is the node on the opposite size.
			other_node = p->nodes[!side];

			// remove the portals from the nodes linked.
			// list ready for when we added the new split nodes.
			RemovePortalFromNode(p, p->nodes[0]);
			RemovePortalFromNode(p, p->nodes[1]);

			// cut the portal into two portals, one on each side of the cut plane
			// this means that the 
			p->pWinding->Split(*plane, SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

			if (frontwinding && frontwinding->IsTiny())
			{
				X_DELETE_AND_NULL(frontwinding, g_arena);
				c_tinyportals++;
			}

			if (backwinding && backwinding->IsTiny())
			{
				X_DELETE_AND_NULL(backwinding, g_arena);
				c_tinyportals++;
			}

			if (!frontwinding && !backwinding)
			{	
				// tiny windings on both sides
				continue;
			}

#if X_DEBUG && 0
			if (frontwinding)
			{
				X_LOG0("Portal", "Split node front winding:");
				frontwinding->Print();
			}
			if (backwinding)
			{
				X_LOG0("Portal", "Split node back winding:");
				backwinding->Print();
			}
#endif // !X_DEBUG

			if (!frontwinding)
			{
				X_DELETE_AND_NULL(backwinding, g_arena);
				if (side == 0)
					AddPortalToNodes(p, b, other_node);
				else
					AddPortalToNodes(p, other_node, b);
				continue;
			}
			if (!backwinding)
			{
				X_DELETE_AND_NULL(frontwinding, g_arena);
				if (side == 0)
					AddPortalToNodes(p, f, other_node);
				else
					AddPortalToNodes(p, other_node, f);
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
				AddPortalToNodes(p, f, other_node);
				AddPortalToNodes(new_portal, b, other_node);
			}
			else
			{
				AddPortalToNodes(p, other_node, f);
				AddPortalToNodes(new_portal, other_node, b);
			}
		}

		node->portals = nullptr;
	}


	bool Portal_Passable(bspPortal  *p)
	{
		if (!p->onNode) {
			return false;	// to global outsideleaf
		}

		if (p->nodes[0]->planenum != PLANENUM_LEAF
			|| p->nodes[1]->planenum != PLANENUM_LEAF)
		{
			X_ERROR("lvl", "Portal_EntityFlood: not a leaf");
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
					LvlBrushSide& side = orig->sides[j];

					// must be visable.
					if (!side.pVisibleHull) {
						continue;
					}

					// portal?
					if (!side.matInfo.getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
						continue;
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
						if (!s2->matInfo.getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
							continue;
						}

						Vec3f center = s2->pVisibleHull->GetCenter();

						X_WARNING("Portal", "brush has multiple area portal sides at (%g,%g,%g)",
							center[0], center[1], center[2]);

						X_DELETE_AND_NULL(s2->pVisibleHull, g_arena);
					}
					return &side;
				}
			}
		}
		return nullptr;
	}

} // namespace


void LvlBuilder::MakeTreePortals_r(bspNode* node)
{
	int i;

	CalcNodeBounds(node);

	if (node->bounds.isEmpty()) {
		X_WARNING("Portal","node without a volume");
	}

	for (i = 0; i < 3; i++) {
		if (node->bounds.min[i] < level::MIN_WORLD_COORD ||
			node->bounds.max[i] > level::MAX_WORLD_COORD) {
			X_WARNING("Portal","node with unbounded volume");
			break;
		}
	}
	if (node->planenum == PLANENUM_LEAF) {
		return;
	}

	MakeNodePortal(planes, node);
	SplitNodePortals(planes, node);

	MakeTreePortals_r(node->children[0]);
	MakeTreePortals_r(node->children[1]);
}

void LvlBuilder::MakeTreePortals(LvlEntity& ent)
{
	MakeHeadnodePortals(ent.bspTree);
	MakeTreePortals_r(ent.bspTree.headnode);
	int break_me = 0;
}



void FloodPortals_r(bspNode *node, int32_t dist) 
{
	bspPortal	*p;
	int			s;

	if (node->occupied) {
		return;
	}

	if (node->opaque) {
		return;
	}

	c_floodedleafs++;
	node->occupied = dist;

	for (p = node->portals; p; p = p->next[s]) {
		s = (p->nodes[1] == node);
		FloodPortals_r(p->nodes[!s], dist + 1);
	}
}


bool LvlBuilder::PlaceOccupant(bspNode* headnode, LvlEntity& ent)
{
	X_ASSERT_NOT_NULL(headnode);
	bspNode* node;
	float	d;

	const Vec3f& origin = ent.origin;

	// find the leaf to start in
	node = headnode;
	while (node->planenum != PLANENUM_LEAF)
	{
		const Planef& plane = planes[node->planenum];
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

	// node->occupant = occupant;
	FloodPortals_r(node, 1);

	return true;
}



bool LvlBuilder::FloodEntities(LvlEntity& ent)
{
	X_LOG0("Lvl", "--- FloodEntities ---");

	bspNode* headnode;
	bspTree* tree;
	bool inside;
	int32_t i;

	tree = &ent.bspTree;
	headnode = tree->headnode;
	inside = false;

	c_floodedleafs = 0;

	// not occupied yet.
	tree->outside_node.occupied = 0;

//	tree->Print(planes);

	// iterate the map ents.
	for (i = 1; i < map_->getNumEntities(); i++)
	{
		mapfile::XMapEntity* mapEnt = map_->getEntity(i);
		LvlEntity& lvlEnt = entities_[i];

		mapfile::XMapEntity::PairIt it = mapEnt->epairs.find("origin");
		if (it == mapEnt->epairs.end()){
			continue;
		}


		if (PlaceOccupant(headnode, lvlEnt)) {
			inside = true;
		}

		// check if the outside nodes has been occupied.
		if (tree->outside_node.occupied) 
		{
			X_ERROR("Lvl", "Leak detected!");
			X_ERROR("Lvl", "Entity: %i", i);
			X_ERROR("Lvl", "origin: %g %g %g", 
				lvlEnt.origin.x,
				lvlEnt.origin.y, 
				lvlEnt.origin.z);

		}

	}

	X_LOG0("Lvl","%5i flooded leafs", c_floodedleafs);


	if (!inside)
	{
		X_ERROR("Lvl", "no entities in open -- no filling");
	}
	else if (tree->outside_node.occupied)
	{
		X_ERROR("Lvl", "entity reached from outside -- no filling");
	}

	return (bool)(inside && !tree->outside_node.occupied);
}


static	size_t		c_outside;
static	size_t		c_inside;
static	size_t		c_solid;

void FillOutside_r(bspNode* node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FillOutside_r(node->children[0]);
		FillOutside_r(node->children[1]);
		return;
	}

	// is this node occupied?
	if (!node->occupied) 
	{
		// if the node is not opaque it must be outside, otherwise it's a solid.
		if (!node->opaque) {
			c_outside++;
			node->opaque = true;
		}
		else {
			c_solid++;
		}
	}
	else {
		c_inside++;
	}

}



bool LvlBuilder::FillOutside(LvlEntity& ent)
{
	c_outside = 0;
	c_inside = 0;
	c_solid = 0;
	X_LOG0("Lvl", "--- FillOutside ---");
	FillOutside_r(ent.bspTree.headnode);
	X_LOG0("Lvl", "%5i solid leafs", c_solid);
	X_LOG0("Lvl", "%5i leafs filled", c_outside);
	X_LOG0("Lvl", "%5i inside leafs", c_inside);

	return true;
}

/// ===========================================

void FloodAreas_r(bspNode *node, size_t area, size_t& areaFloods)
{
	bspPortal* p;
	bspNode* other;
	int	s;

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

bool CheckAreas_r(bspNode* node)
{
	if (node->planenum != PLANENUM_LEAF) 
	{
		if (!CheckAreas_r(node->children[0]))
			return false;
		if (!CheckAreas_r(node->children[1]))
			return false;

		return true;
	}

	if (!node->opaque && node->area < 0) {
		X_ERROR("Portal", "node has a invalid area: %i", node->area);
		return false;
	}

	return true;
}





bool LvlBuilder::FloodAreas(LvlEntity& ent)
{
	X_LOG0("Lvl", "--- FloodAreas ---");

	size_t numAreas = 0;
	// find how many we have.
	FindAreas_r(ent.bspTree.headnode, numAreas);

	X_LOG0("Lvl", "%5i areas", numAreas);

	ent.numAreas = numAreas;

	// check we not missed.
	if (!CheckAreas_r(ent.bspTree.headnode))
		return false;

	// skip inter area portals if only one?
	if (numAreas < 2) {
		X_LOG0("Portal", "Skipping inter portals. less than two area's");
		return true;
	}

	// we want to create inter area portals now.
	if (!ent.FindInterAreaPortals()) {
		X_ERROR("Portal", "Failed to calculate the inter area portal info.");
		return false;
	}

	return true;
}


// =============================================================================

void bspPortal::MakeNodePortal(XPlaneSet& planeSet, bspNode* node)
{
	bspPortal	*new_portal, *p;
	XWinding	*w;
	Vec3f		normal;
	int			side;

	w = BaseWindingForNode(planeSet, node);

	// clip the portal by all the other portals in the node
	for (p = node->portals; p && w; p = p->next[side])
	{
		Planef	plane;

		if (p->nodes[0] == node)
		{
			side = 0;
			plane = p->plane;
		}
		else if (p->nodes[1] == node)
		{
			side = 1;
			plane = -p->plane;
		}
		else {
			X_ERROR("Portal", "CutNodePortals_r: mislinked portal");
			side = 0;	// quiet a compiler warning
		}

		w = w->Clip(plane, CLIP_EPSILON);
	}

	if (!w)
	{
		X_WARNING("Portal", "Winding is empty");
		return;
	}

#if X_DEBUG
	w->Print();
#endif // X_DEBUG

	if (w->IsTiny())
	{
		c_tinyportals++;
		X_DELETE(w, g_arena);
		return;
	}

	new_portal = X_NEW(bspPortal, g_arena, "Portal");
	new_portal->plane = planeSet[node->planenum];
	new_portal->onNode = node;
	new_portal->pWinding = w;
	new_portal->AddToNodes(node->children[0], node->children[1]);
}



bool bspPortal::MakeTreePortals(XPlaneSet& planeSet, LvlEntity* pEnt)
{
	X_ASSERT_NOT_NULL(pEnt);
	X_ASSERT_NOT_NULL(pEnt->bspTree.headnode);

	MakeHeadnodePortals(pEnt->bspTree);
	pEnt->bspTree.headnode->MakeTreePortals_r(planeSet);
	return true;
}

void bspPortal::MakeTreePortals_r(XPlaneSet& planeSet, bspNode* pNode)
{
	X_ASSERT_NOT_NULL(pNode);
	size_t i;

	pNode->CalcNodeBounds();

	if (pNode->bounds.isEmpty()) {
		X_WARNING("Portal", "node without a volume");
	}

	for (i = 0; i < 3; i++) 
	{
		if (pNode->bounds.min[i] < level::MIN_WORLD_COORD ||
			pNode->bounds.max[i] > level::MAX_WORLD_COORD) 
		{
			X_WARNING("Portal", "node with unbounded volume");
			break;
		}
	}

	if (pNode->planenum == PLANENUM_LEAF) {
		return;
	}

	MakeNodePortal(planeSet, pNode);
	SplitNodePortals(planeSet, pNode);

	MakeTreePortals_r(planeSet, pNode->children[0]);
	MakeTreePortals_r(planeSet, pNode->children[1]);
}

void bspPortal::MakeHeadnodePortals(bspTree& tree)
{
	AABB	bounds;
	int		i, j, n;
	Planef	bplanes[6], *pl;
	bspNode* node;
	bspPortal *p, *portals[6];


	node = tree.headnode;

	tree.outside_node.planenum = PLANENUM_LEAF;
	//		tree->outside_node.brushlist = NULL;
	tree.outside_node.portals = nullptr;
	tree.outside_node.opaque = false;

	// if no nodes, don't go any farther
	if (node->planenum == PLANENUM_LEAF) {
		return;
	}

	// pad with some space so there will never be null volume leafs
	for (i = 0; i<3; i++) {
		bounds.min[i] = tree.bounds.min[i] - SIDESPACE;
		bounds.max[i] = tree.bounds.max[i] + SIDESPACE;
		if (bounds.min[i] >= bounds.max[i]) {
			X_FATAL("BspPortal", "Backward tree volume");
		}
	}

	for (i = 0; i<3; i++) {
		for (j = 0; j<2; j++) {
			n = j * 3 + i;

			p = X_NEW(bspPortal, g_arena, "bspHeadPortal");
			portals[n] = p;

			pl = &bplanes[n];
			memset(pl, 0, sizeof(*pl));

			if (j) {
				(*pl)[i] = -1;
				(*pl).setDistance(-bounds.max[i]);
			}
			else {
				(*pl)[i] = 1;
				(*pl).setDistance(bounds.min[i]);
			}

			p->plane = *pl;
			p->pWinding = X_NEW(XWinding, g_arena, "bspPortalWinding")(*pl);

			AddPortalToNodes(p, node, &tree.outside_node);
		}
	}

	// clip the basewindings by all the other planes
	for (i = 0; i<6; i++)
	{
		for (j = 0; j<6; j++) 
		{
			if (j == i) {
				continue;
			}
			portals[i]->pWinding = portals[i]->pWinding->Clip(bplanes[j], ON_EPSILON);
		}
	}

#if X_DEBUG
	X_LOG0("BspPortal", "Head node windings");
	// print the head nodes portal bounds.
	for (i = 0; i<6; i++) {
		portals[i]->pWinding->Print();
	}
#endif // !X_DEBUG
}


void bspPortal::AddToNodes(bspNode* pFront, bspNode* pBack)
{
	X_ASSERT_NOT_NULL(this);

	if (nodes[0] || nodes[1])
	{
		X_ERROR("Portal", "Node already included");
	}

	nodes[0] = pFront;
	next[0] = pFront->portals;
	pFront->portals = this;

	nodes[1] = pBack;
	next[1] = pBack->portals;
	pBack->portals = this;
}

void bspPortal::RemoveFromNode(bspNode* pNode)
{
	bspPortal* pPortal = this;
	bspPortal	**pp, *t;

	// remove reference to the current portal
	pp = &pNode->portals;

	while (1)
	{
		t = *pp;

		if (!t) {
			X_ERROR("Portal", "RemovePortalFromNode: portal not in leaf");
		}

		if (t == pPortal) {
			break;
		}

		if (t->nodes[0] == pNode)
			pp = &t->next[0];
		else if (t->nodes[1] == pNode)
			pp = &t->next[1];
		else {
			X_ERROR("Portal", "RemovePortalFromNode: portal not bounding leaf");
		}
	}

	if (pPortal->nodes[0] == pNode) {
		*pp = pPortal->next[0];
		pPortal->nodes[0] = nullptr;
	}
	else if (pPortal->nodes[1] == pNode) {
		*pp = pPortal->next[1];
		pPortal->nodes[1] = nullptr;
	}
	else {
		X_ERROR("Portal", "RemovePortalFromNode: mislinked");
	}
}

const LvlBrushSide* bspPortal::FindAreaPortalSide(void) const
{
	size_t			i, x, j, k;
	bspNode			*node;
	LvlBrush		*b, *orig;
	LvlBrushSide	*s2;

	// scan both bordering nodes brush lists for a portal brush
	// that shares the plane
	for (i = 0; i < 2; i++)
	{
		node = nodes[i];
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
				LvlBrushSide& side = orig->sides[j];

				// must be visable.
				if (!side.pVisibleHull) {
					continue;
				}

				// portal?
				if (!side.matInfo.getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
					continue;
				}

				if ((side.planenum & ~1) != (onNode->planenum & ~1)) {
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
					if (!s2->matInfo.getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
						continue;
					}

					Vec3f center = s2->pVisibleHull->GetCenter();

					X_WARNING("Portal", "brush has multiple area portal sides at (%g,%g,%g)",
						center[0], center[1], center[2]);

					X_DELETE_AND_NULL(s2->pVisibleHull, g_arena);
				}
				return &side;
			}
		}
	}
	return nullptr;
}

bool bspPortal::HasAreaPortalSide(void) const
{
	return FindAreaPortalSide() != nullptr;
}


bool bspPortal::PortalPassable(void) const
{
	if (!onNode) {
		return false;	// to global outsideleaf
	}

	if (nodes[0]->planenum != PLANENUM_LEAF
		|| nodes[1]->planenum != PLANENUM_LEAF)
	{
		X_ERROR("bspPortal", "not a leaf");
	}

	if (!nodes[0]->opaque && !nodes[1]->opaque) {
		return true;
	}

	return false;
}
