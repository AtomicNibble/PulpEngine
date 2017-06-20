#include "stdafx.h"
#include "LvlBuilder.h"

#include "MapLoader.h"

#include <IConsole.h>

namespace
{
	#define	SIDESPACE	8


} // namespace


X_NAMESPACE_BEGIN(lvl)

// =============================================================================

void bspPortal::MakeNodePortal(XPlaneSet& planeSet, bspNode* node)
{
	bspPortal	*new_portal, *p;
	XWinding	*w;
	Vec3f		normal;
	int			side;

	w = node->GetBaseWinding(planeSet);

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
			X_ERROR("BspPortal", "CutNodePortals_r: mislinked portal");
			side = 0;	// quiet a compiler warning
		}

		if (!w->clip(plane, CLIP_EPSILON)) {
			X_DELETE_AND_NULL(w, g_arena);
		}
	}

	if (!w)
	{
		X_WARNING("BspPortal", "Winding is empty");
		return;
	}

#if X_DEBUG && 0
	w->Print();
#endif // X_DEBUG

	if (w->isTiny())
	{
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
	X_ASSERT_NOT_NULL(pEnt->bspTree_.headnode);

	MakeHeadnodePortals(pEnt->bspTree_);
	pEnt->bspTree_.headnode->MakeTreePortals_r(planeSet);
	return true;
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
			p->AddToNodes(node, &tree.outside_node);
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
			if(!portals[i]->pWinding->clip(bplanes[j], ON_EPSILON)){
				X_DELETE_AND_NULL(portals[i]->pWinding, g_arena);
			}
		}
	}

	core::ICVar* pLogVerbosity = gEnv->pConsole->GetCVar("log_verbosity");
	if (!pLogVerbosity || pLogVerbosity->GetInteger() >= 1)
	{
		X_LOG1("BspPortal", "Head node windings");
		// print the head nodes portal bounds.
		for (i = 0; i < 6; i++) {
			portals[i]->pWinding->print();
		}
	}
}


void bspPortal::AddToNodes(bspNode* pFront, bspNode* pBack)
{
	X_ASSERT_NOT_NULL(this);

	if (nodes[0] || nodes[1])
	{
		X_ERROR("BspPortal", "Node already included");
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
			X_ERROR("BspPortal", "RemovePortalFromNode: portal not in leaf");
		}

		if (t == pPortal) {
			break;
		}

		if (t->nodes[0] == pNode)
			pp = &t->next[0];
		else if (t->nodes[1] == pNode)
			pp = &t->next[1];
		else {
			X_ERROR("BspPortal", "RemovePortalFromNode: portal not bounding leaf");
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
			for (j = 0; j < orig->sides.size(); j++)
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

					Vec3f center = s2->pVisibleHull->getCenter();

					X_WARNING("BspPortal", "brush has multiple area portal sides at (%g,%g,%g)",
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

X_NAMESPACE_END