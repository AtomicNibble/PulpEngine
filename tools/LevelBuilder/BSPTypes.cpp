#include "stdafx.h"
#include "BSPTypes.h"


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

bspNode::bspNode()
{
	planenum = -1;

	parent = nullptr;
	children[0] = nullptr;
	children[1] = nullptr;
	portals = nullptr;

	tinyportals = 0;

	opaque = true;
	areaportal = false;

	cluster = -1;
	area = -1;
	occupied = -1;
}

void bspNode::TreePrint_r(const XPlaneSet& planes, size_t depth) const
{
	X_LOG0("bspNode", "Depth: %i", depth);

	// is this a leaf node.
	if (this->planenum == PLANENUM_LEAF)
	{
		X_LOG0("bspNode", "LEAF");
		return;
	}

	// print the plane.
	const Planef& plane = planes[this->planenum];
	const Vec3f& Pn = plane.getNormal();

	X_LOG0("bspNode", "PlaneNum: %i Plane: (%g,%g,%g) %g",
		this->planenum, Pn[0], Pn[1], Pn[2], plane.getDistance());

	// if it's not a leaf node then we should not have any null children.
	X_ASSERT_NOT_NULL(children[0]);
	X_ASSERT_NOT_NULL(children[1]);

	// print the children.	
	depth++;

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