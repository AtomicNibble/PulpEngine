#include "stdafx.h"
#include "LevelBuilder.h"

namespace
{
	#define	SIDESPACE	8

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

}


void LvlBuilder::MakeTreePortals(LvlEntity& ent)
{
	MakeHeadnodePortals(ent.bspTree);


}