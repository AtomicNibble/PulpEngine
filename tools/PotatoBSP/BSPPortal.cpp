#include "stdafx.h"

#include <ITimer.h>
#include <IProfile.h>

#include "BSPTypes.h"
#include "MapTypes.h"

#if 0

int		c_active_portals;
int		c_peak_portals;
int		c_tinyportals;

#define	MAX_INTER_AREA_PORTALS	1024

typedef struct {
	int		area0, area1;
	side_t	*side;
} interAreaPortal_t;

interAreaPortal_t interAreaPortals[MAX_INTER_AREA_PORTALS];
int					numInterAreaPortals;


static	int		c_areas;
static	int		c_areaFloods;



uPortal_t* AllocPortal(void)
{
	uPortal_t	*p;

	c_active_portals++;
	if (c_active_portals > c_peak_portals)
		c_peak_portals = c_active_portals;

	p = (uPortal_t *)malloc(sizeof(uPortal_t));
	memset(p, 0, sizeof(uPortal_t));

	return p;
}


void FreePortal(uPortal_t  *p)
{
	if (p->winding)
		delete p->winding;
	c_active_portals--;
	free(p);
}

void PrintPortal(uPortal_t *p)
{
	int			i;
	XWinding	*w;

	w = p->winding;

	X_LOG0("Portal", "Winding");
	for (i = 0; i < w->GetNumPoints(); i++)
		X_LOG0("Portal", "(%5.0f,%5.0f,%5.0f)", (*w)[i][0], (*w)[i][1], (*w)[i][2]);

}


void BSPData::AddPortalToNodes(uPortal_t  *p, node_t *front, node_t *back)
{
	X_ASSERT(!(p->nodes[0] || p->nodes[1]), "allready included")();

	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;

	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
}


/*
================
MakeHeadnodePortals

The created portals will face the global outside_node
================
*/
#define	SIDESPACE	8
void BSPData::MakeHeadnodePortals(tree_t *tree)
{
	AABB		bounds;
	int			i, j, n;
	uPortal_t	*p, *portals[6];
	Planef		bplanes[6], *pl;
	node_t *node;

	node = tree->headnode;

	tree->outside_node.planenum = PLANENUM_LEAF;
	tree->outside_node.brushlist = NULL;
	tree->outside_node.portals = NULL;
	tree->outside_node.opaque = false;

	// if no nodes, don't go any farther
	if (node->planenum == PLANENUM_LEAF) {
		return;
	}

	// pad with some space so there will never be null volume leafs
	for (i = 0; i<3; i++) {
		bounds.min[i] = tree->bounds.min[i] - SIDESPACE;
		bounds.max[i] = tree->bounds.max[i] + SIDESPACE;
		if (bounds.min[i] >= bounds.max[i]) {
			X_ERROR("Portal", "Backwards tree volume");
		}
	}

	Vec3f boundsVec;
	for (i = 0; i<3; i++) {
		for (j = 0; j<2; j++) {
			n = j * 3 + i;

			boundsVec = (j == 0) ? bounds.min : bounds.max;

			p = AllocPortal();
			portals[n] = p;

			pl = &bplanes[n];
			core::zero_this(pl);

			if (j) {
				(*pl)[i] = -1;
				(*pl).setDistance(-boundsVec[i]);
			}
			else {
				(*pl)[i] = 1;
				(*pl).setDistance(boundsVec[i]);
			}
			p->plane = *pl;
			p->winding = new XWinding(*pl);
			AddPortalToNodes(p, node, &tree->outside_node);
		}
	}

	// clip the basewindings by all the other planes
	for (i = 0; i<6; i++) {
		for (j = 0; j<6; j++) {
			if (j == i) {
				continue;
			}
			portals[i]->winding = portals[i]->winding->Clip(bplanes[j], ON_EPSILON);
		}
	}
}



void BSPData::CalcNodeBounds(node_t *node)
{
	uPortal_t	*p;
	int			s;
	int			i;

	// calc mins/maxs for both leafs and nodes
	node->bounds.clear();
	for (p = node->portals; p; p = p->next[s]) {
		s = (p->nodes[1] == node);
		for (i = 0; i < p->winding->GetNumPoints(); i++) {
			node->bounds.add((*p->winding)[i]);
		}
	}
}

void BSPData::RemovePortalFromNode(uPortal_t  *portal, node_t *l)
{
	uPortal_t	**pp, *t;

	// remove reference to the current portal
	pp = &l->portals;
	while (1)
	{
		t = *pp;
		if (!t)
			X_FATAL("Portal", "RemovePortalFromNode: portal not in leaf");

		if (t == portal)
			break;

		if (t->nodes[0] == l)
			pp = &t->next[0];
		else if (t->nodes[1] == l)
			pp = &t->next[1];
		else
			X_FATAL("Portal", "RemovePortalFromNode: portal not bounding leaf");
	}

	if (portal->nodes[0] == l) {
		*pp = portal->next[0];
		portal->nodes[0] = NULL;
	}
	else if (portal->nodes[1] == l) {
		*pp = portal->next[1];
		portal->nodes[1] = NULL;
	}
	else {
		X_FATAL("Portal", "RemovePortalFromNode: mislinked");
	}
}


#define	BASE_WINDING_EPSILON	0.001f
#define	SPLIT_WINDING_EPSILON	0.001f

XWinding* BSPData::BaseWindingForNode(node_t *node)
{
	XWinding	*w;
	node_t		*n;

	w = new XWinding(planes[node->planenum]);

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

//============================================================

/*
==================
MakeNodePortal

create the new portal by taking the full plane winding for the cutting plane
and clipping it by all of parents of this node
==================
*/
void BSPData::MakeNodePortal(node_t *node)
{
	uPortal_t	*new_portal, *p;
	XWinding	*w;
	Vec3f		normal;
	int			side;

	w = BaseWindingForNode(node);

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
			X_FATAL("Portal", "CutNodePortals_r: mislinked portal");
			side = 0;	// quiet a compiler warning
		}

		w = w->Clip(plane, CLIP_EPSILON);
	}

	if (!w)
	{
		return;
	}

	if (w->IsTiny())
	{
		c_tinyportals++;
		delete w;
		return;
	}

	new_portal = AllocPortal();
	new_portal->plane = planes[node->planenum];
	new_portal->onnode = node;
	new_portal->winding = w;
	AddPortalToNodes(new_portal, node->children[0], node->children[1]);

//	PrintPortal(new_portal);
}

void BSPData::SplitNodePortals(node_t *node)
{
	uPortal_t	*p, *next_portal, *new_portal;
	node_t		*f, *b, *other_node;
	int			side;
	Planef		*plane;
	XWinding	*frontwinding, *backwinding;

	plane = &planes[node->planenum];
	f = node->children[0];
	b = node->children[1];

	for (p = node->portals; p; p = next_portal) {
		if (p->nodes[0] == node) {
			side = 0;
		}
		else if (p->nodes[1] == node) {
			side = 1;
		}
		else {
			X_FATAL("Portal", "SplitNodePortals: mislinked portal");
			side = 0;	// quiet a compiler warning
		}
		next_portal = p->next[side];

		other_node = p->nodes[!side];
		RemovePortalFromNode(p, p->nodes[0]);
		RemovePortalFromNode(p, p->nodes[1]);

		//
		// cut the portal into two portals, one on each side of the cut plane
		//
		p->winding->Split(*plane, SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

		if (frontwinding && frontwinding->IsTiny())
		{
			delete frontwinding;
			frontwinding = NULL;
			c_tinyportals++;
		}

		if (backwinding && backwinding->IsTiny())
		{
			delete backwinding;
			backwinding = NULL;
			c_tinyportals++;
		}

		if (!frontwinding && !backwinding)
		{	// tiny windings on both sides
			continue;
		}

		if (!frontwinding)
		{
			delete backwinding;
			if (side == 0)
				AddPortalToNodes(p, b, other_node);
			else
				AddPortalToNodes(p, other_node, b);
			continue;
		}
		if (!backwinding)
		{
			delete frontwinding;
			if (side == 0)
				AddPortalToNodes(p, f, other_node);
			else
				AddPortalToNodes(p, other_node, f);
			continue;
		}

		// the winding is split
		new_portal = AllocPortal();
		*new_portal = *p;
		new_portal->winding = backwinding;
		delete p->winding;
		p->winding = frontwinding;

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

	node->portals = NULL;
}




void BSPData::MakeTreePortals_r(node_t *node)
{
	int		i;

	CalcNodeBounds(node);

	if (node->bounds.min[0] >= node->bounds.max[0]) {
		X_WARNING("Potal", "node without a volume");
	}

	for (i = 0; i < 3; i++) {
		if (node->bounds.min[i] < bsp::MIN_WORLD_COORD || node->bounds.max[i] > bsp::MAX_WORLD_COORD) {
			X_WARNING("Potal", "node with unbounded volume");
			break;
		}
	}
	if (node->planenum == PLANENUM_LEAF) {
		return;
	}

	MakeNodePortal(node);
	SplitNodePortals(node);

	MakeTreePortals_r(node->children[0]);
	MakeTreePortals_r(node->children[1]);
}



void BSPData::MakeTreePortals(tree_t *tree)
{
	X_LOG0("Portals", "----- MakeTreePortals -----");

	MakeHeadnodePortals(tree);
	MakeTreePortals_r(tree->headnode);
}

// -------------------------------------------------------

int		c_floodedleafs;
int		c_outside;
int		c_inside;
int		c_solid;



bool BSPData::FloodEntities(tree_t *tree)
{
	int		i;
	Vec3f	origin;
	const char	*cl;
	bool	inside;
	node_t *headnode;

	headnode = tree->headnode;
	X_LOG0("Portals", "--- FloodEntities ---");
	inside = false;
	tree->outside_node.occupied = 0;

	c_floodedleafs = 0;
	bool errorShown = false;
	bool infoPlayerFnd = false;

	for (i = 1; i<numEntities; i++) 
	{
		mapfile::XMapEntity	*mapEnt;

		mapEnt = entities[i].mapEntity;

		if (!mapEnt->epairs.GetVector("origin", "", origin)) {
			continue;
		}

	//	origin[2] += 1;	// so objects on floor are ok

		mapEnt->epairs.GetString("classname", "", &cl);

		// nudge playerstart around if needed so clipping hulls allways
		// have a valid point
		if (!strcmp(cl, "info_player_start"))
		{
			infoPlayerFnd = true;

			int	x, y;
			for (x = -16; x <= 16; x += 16)
			{
				for (y = -16; y <= 16; y += 16)
				{
					origin[0] += x;
					origin[1] += y;
					if (PlaceOccupant(headnode, origin, &entities[i]))
					{
						inside = true;
						goto gotit;
					}
					origin[0] -= x;
					origin[1] -= y;
				}
			}
		gotit:;
		}
		else
		{
			if (PlaceOccupant(headnode, origin, &entities[i]))
				inside = true;
		}


		if (tree->outside_node.occupied && !errorShown) 
		{
			errorShown = true;
			X_WARNING("Portals", "Leak on entity # %d", i);
			const char *p;

			mapEnt->epairs.GetString("classname", "", &p);
			X_WARNING("Portals", "Entity classname: %s", p);
			mapEnt->epairs.GetString("targetname", "", &p);
			X_WARNING("Portals", "Entity targetname: %s", p);

			mapEnt->epairs.GetVector("origin", "", origin);

			X_WARNING("Portals", "Entity origin: %f %f %f", origin.x, origin.y, origin.z);
		}
	}

	X_LOG0("Portals", "%5i flooded leafs", c_floodedleafs);

	if (!infoPlayerFnd)
	{
		X_ERROR("Portals", "no \"info_player_start\" found");
	}

	if (!inside)
	{
		X_WARNING("Portals", "no entities in open -- no filling");
	}
	else if (tree->outside_node.occupied)
	{
		X_WARNING("Portals", "entity reached from outside -- no filling");
	}

	return (bool)(inside && !tree->outside_node.occupied);
}

bool BSPData::PlaceOccupant(node_t* headnode, Vec3f& origin, uEntity_t* occupant)
{
	node_t	*node;
	float	d;
	Planef	*plane;

	// find the leaf to start in
	node = headnode;
	while (node->planenum != PLANENUM_LEAF) 
	{
		plane = &planes[node->planenum];
		d = plane->distance(origin);
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

	node->occupant = occupant;
	FloodPortals_r(node, 1);
	return true;
}

void BSPData::FloodPortals_r(node_t *node, int dist)
{
	uPortal_t	*p;
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



void BSPData::FillOutside_r(node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FillOutside_r(node->children[0]);
		FillOutside_r(node->children[1]);
		return;
	}

	// anything not reachable by an entity
	// can be filled away
	if (!node->occupied) {
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

/*
=============
FillOutside

Fill (set node->opaque = true) all nodes that can't be reached by entities
=============
*/
void BSPData::FillOutside(uEntity_t *e)
{
	c_outside = 0;
	c_inside = 0;
	c_solid = 0;
	X_LOG0("Portals", "--- FillOutside ---");
	FillOutside_r(e->tree->headnode);
	X_LOG0("Portals", "%5i solid leafs", c_solid);
	X_LOG0("Portals", "%5i leafs filled", c_outside);
	X_LOG0("Portals", "%5i inside leafs", c_inside);
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------


static side_t* FindSideForPortal(uPortal_t* p) 
{
#if 0
	int		i, j, k;
	node_t	*node;
	uBrush_t	*b, *orig;
	side_t	*s, *s2;

	// scan both bordering nodes brush lists for a portal brush
	// that shares the plane
	for (i = 0; i < 2; i++) {
		node = p->nodes[i];
		for (b = node->brushlist; b; b = b->next) {
			if (!(b->contents & CONTENTS_AREAPORTAL)) {
				continue;
			}
			orig = b->original;
			for (j = 0; j < orig->numsides; j++) {
				s = orig->sides + j;
				if (!s->visibleHull) {
					continue;
				}
				//if (!(s->material->GetContentFlags() & CONTENTS_AREAPORTAL)) {
				//	continue;
				//}

				if ((s->planenum & ~1) != (p->onnode->planenum & ~1)) {
					continue;
				}
				// remove the visible hull from any other portal sides of this portal brush
				for (k = 0; k < orig->numsides; k++) {
					if (k == j) {
						continue;
					}
					s2 = orig->sides + k;
					if (s2->visibleHull == NULL) {
						continue;
					}
				//	if (!(s2->material->GetContentFlags() & CONTENTS_AREAPORTAL)) {
				//		continue;
				//	}

				//	common->Warning("brush has multiple area portal sides at %s", s2->visibleHull->GetCenter().ToString());
				// TODO
					delete s2->visibleHull;
					s2->visibleHull = NULL;
				}
				return s;
			}
		}
	}
#endif
	return nullptr;
}



/*
=============
Portal_Passable

Returns true if the portal has non-opaque leafs on both sides
=============
*/
static bool Portal_Passable(uPortal_t  *p) 
{
	if (!p->onnode) {
		return false;	// to global outsideleaf
	}

	if (p->nodes[0]->planenum != PLANENUM_LEAF
		|| p->nodes[1]->planenum != PLANENUM_LEAF) {
		X_ERROR("Portal", "Portal_EntityFlood: not a leaf");
	}

	if (!p->nodes[0]->opaque && !p->nodes[1]->opaque) {
		return true;
	}

	return false;
}


void FloodAreas_r(node_t* node)
{
	uPortal_t	*p;
	int			s;

	if (node->area != -1) {
		return;		// allready got it
	}
	if (node->opaque) {
		return;
	}

	c_areaFloods++;
	node->area = c_areas;

	for (p = node->portals; p; p = p->next[s]) {
		node_t	*other;

		s = (p->nodes[1] == node);
		other = p->nodes[!s];

		if (!Portal_Passable(p)) {
			continue;
		}

		// can't flood through an area portal
		if (FindSideForPortal(p)) {
			continue;
		}

		FloodAreas_r(other);
	}
}

/*
=============
FindAreas_r

Just decend the tree, and for each node that hasn't had an
area set, flood fill out from there
=============
*/
void FindAreas_r(node_t *node) {
	if (node->planenum != PLANENUM_LEAF) {
		FindAreas_r(node->children[0]);
		FindAreas_r(node->children[1]);
		return;
	}

	if (node->opaque) {
		return;
	}

	if (node->area != -1) {
		return;		// allready got it
	}

	c_areaFloods = 0;
	FloodAreas_r(node);
	X_LOG0("Portal", "area %i has %i leafs", c_areas, c_areaFloods);
	c_areas++;
}

/*
============
CheckAreas_r
============
*/
void CheckAreas_r(node_t *node) {
	if (node->planenum != PLANENUM_LEAF) {
		CheckAreas_r(node->children[0]);
		CheckAreas_r(node->children[1]);
		return;
	}
	if (!node->opaque && node->area < 0) {
		X_ERROR("Portal", "CheckAreas_r: area = %i", node->area);
	}
}

/*
============
ClearAreas_r

Set all the areas to -1 before filling
============
*/
void ClearAreas_r(node_t *node) {
	if (node->planenum != PLANENUM_LEAF) {
		ClearAreas_r(node->children[0]);
		ClearAreas_r(node->children[1]);
		return;
	}
	node->area = -1;
}

//=============================================================


/*
=================
FindInterAreaPortals_r

=================
*/
static void FindInterAreaPortals_r(node_t *node) {
	uPortal_t	*p;
	int			s;
	int			i;
	XWinding	*w;
	interAreaPortal_t	*iap;
	side_t		*side;

	if (node->planenum != PLANENUM_LEAF) {
		FindInterAreaPortals_r(node->children[0]);
		FindInterAreaPortals_r(node->children[1]);
		return;
	}

	if (node->opaque) {
		return;
	}

	for (p = node->portals; p; p = p->next[s]) {
		node_t	*other;

		s = (p->nodes[1] == node);
		other = p->nodes[!s];

		if (other->opaque) {
			continue;
		}

		// only report areas going from lower number to higher number
		// so we don't report the portal twice
		if (other->area <= node->area) {
			continue;
		}

		side = FindSideForPortal(p);
		//		w = p->winding;
		if (!side) {
	//		common->Warning("FindSideForPortal failed at %s", p->winding->GetCenter().ToString());
			continue;
		}
		w = side->visibleHull;
		if (!w) {
			continue;
		}

		// see if we have created this portal before
		for (i = 0; i < numInterAreaPortals; i++) {
			iap = &interAreaPortals[i];

			if (side == iap->side &&
				((p->nodes[0]->area == iap->area0 && p->nodes[1]->area == iap->area1)
				|| (p->nodes[1]->area == iap->area0 && p->nodes[0]->area == iap->area1))) {
				break;
			}
		}

		if (i != numInterAreaPortals) {
			continue;	// already emited
		}

		iap = &interAreaPortals[numInterAreaPortals];
		numInterAreaPortals++;
		if (side->planenum == p->onnode->planenum) {
			iap->area0 = p->nodes[0]->area;
			iap->area1 = p->nodes[1]->area;
		}
		else {
			iap->area0 = p->nodes[1]->area;
			iap->area1 = p->nodes[0]->area;
		}
		iap->side = side;

	}
}


/*
=============
FloodAreas

Mark each leaf with an area, bounded by CONTENTS_AREAPORTAL
Sets e->areas.numAreas
=============
*/
void BSPData::FloodAreas(uEntity_t *e)
{
//	common->Printf("--- FloodAreas ---\n");
	X_LOG0("Portal", "--- FloodAreas ---");

	// set all areas to -1
	ClearAreas_r(e->tree->headnode);

	// flood fill from non-opaque areas
	c_areas = 0;
	FindAreas_r(e->tree->headnode);

//	common->Printf("%5i areas\n", c_areas);
	X_LOG0("Portal", "%5i areas", c_areas);
	e->numAreas = c_areas;

	// make sure we got all of them
	CheckAreas_r(e->tree->headnode);

	// identify all portals between areas if this is the world
	if (e == &entities[0]) {
		numInterAreaPortals = 0;
		FindInterAreaPortals_r(e->tree->headnode);
	}
}

#endif