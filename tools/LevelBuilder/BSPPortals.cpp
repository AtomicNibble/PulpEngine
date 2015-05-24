#include "stdafx.h"
#include "LevelBuilder.h"

#include "MapLoader.h"

namespace
{
	#define	SIDESPACE	8

	size_t c_tinyportals = 0;

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
				X_ERROR("Bsp", "Backward tree volume");
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

	#define	BASE_WINDING_EPSILON	0.001f
	#define	SPLIT_WINDING_EPSILON	0.001f

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

		plane = &planes[node->planenum];
		f = node->children[0];
		b = node->children[1];

		for (p = node->portals; p; p = next_portal) 
		{
			if (p->nodes[0] == node) {
				side = 0;
			}
			else if (p->nodes[1] == node) {
				side = 1;
			}
			else {
				X_ERROR("Portla", "SplitNodePortals: mislinked portal");
				side = 0;	// quiet a compiler warning
			}
			next_portal = p->next[side];

			other_node = p->nodes[!side];

			RemovePortalFromNode(p, p->nodes[0]);
			RemovePortalFromNode(p, p->nodes[1]);

			//
			// cut the portal into two portals, one on each side of the cut plane
			//
			p->pWinding->Split(*plane, SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

			if (frontwinding && frontwinding->IsTiny())
			{
				X_DELETE_AND_NULL( frontwinding, g_arena);
				c_tinyportals++;
			}

			if (backwinding && backwinding->IsTiny())
			{
				X_DELETE_AND_NULL(backwinding, g_arena);
				c_tinyportals++;
			}

			if (!frontwinding && !backwinding)
			{	// tiny windings on both sides
				continue;
			}

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
			new_portal = X_NEW(bspPortal, g_arena, "Portal");
			*new_portal = *p;
			new_portal->pWinding = backwinding;
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


size_t c_floodedleafs = 0;

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

	tree->Print(planes);

	// iterate the map ents.
	for (i = 0; i < map_->getNumEntities(); i++)
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

