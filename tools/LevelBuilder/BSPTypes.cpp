#include "stdafx.h"
#include "BSPTypes.h"


FillStats::FillStats()
{
	core::zero_this(this);
}


void FillStats::print(void) const
{
	X_LOG0("FillStats", "%5i solid leafs", numSolid);
	X_LOG0("FillStats", "%5i leafs filled", numOutside);
	X_LOG0("FillStats", "%5i inside leafs", numInside);
}

// ------------------------------ Face -----------------------------------

bspFace::bspFace()
{
	pNext = nullptr;

	planenum = -1;
	portal = false;
	checked = false;

	w = nullptr;
}

// ------------------------------ Portal -----------------------------------

bspPortal::bspPortal()
{
	onNode = nullptr;
	core::zero_object(nodes);
	core::zero_object(next);
	pWinding = nullptr;
}

// ------------------------------ Node -----------------------------------

bspNode::bspNode() :
brushes(g_arena)
{
	planenum = 0;

	parent = nullptr;
	children[0] = nullptr;
	children[1] = nullptr;
	portals = nullptr;

	opaque = false;

	area = -1;
	occupied = 0;
}

void bspNode::TreePrint_r(const XPlaneSet& planes, size_t depth) const
{
	X_LOG0("bspNode", "==========================");
	X_LOG0("bspNode", "Depth: %i", depth);

	// is this a leaf node.
	if (this->planenum == PLANENUM_LEAF)
	{
		X_LOG0("bspNode", "LEAF");
		X_LOG0("bspNode", "==========================");
		return;
	}

	// print the plane.
	const Planef& plane = planes[this->planenum];
	const Vec3f& Pn = plane.getNormal();

	X_LOG0("bspNode", "Opaque: %i", this->opaque);
	X_LOG0("bspNode", "Occupied: %i", this->occupied);
	X_LOG0("bspNode", "Area: %i", this->area);
//	X_LOG0("bspNode", "tinyportals: %i", this->tinyportals);

	X_LOG0("bspNode", "PlaneNum: %i Plane: (%g,%g,%g) %g",
		this->planenum, Pn[0], Pn[1], Pn[2], plane.getDistance());

	// if it's not a leaf node then we should not have any null children.
	X_ASSERT_NOT_NULL(children[0]);
	X_ASSERT_NOT_NULL(children[1]);

	// print the children.	
	depth++;

	X_LOG0("bspNode", "==========================");


	children[0]->TreePrint_r(planes, depth);
	children[1]->TreePrint_r(planes, depth);
}

// ------------------------------ Tree -----------------------------------

bspTree::bspTree()
{
	headnode = nullptr;
}

void bspTree::Print(const XPlaneSet& planes) const
{
	// print me baby!
	X_LOG0("BspTree", "Printing tree:");
	if (headnode) {
		size_t depth = 0;
		headnode->TreePrint_r(planes, depth);
	}
}