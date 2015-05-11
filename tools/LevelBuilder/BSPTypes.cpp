#include "stdafx.h"
#include "BSPTypes.h"


// ------------------------------ Face -----------------------------------

bspFace::bspFace()
{
	planenum = -1;
	portal = false;
	checked = false;

	w = nullptr;
}

// ------------------------------ Node -----------------------------------

bspNode::bspNode()
{
	planenum = -1;

	parent = nullptr;
	children[0] = nullptr;
	children[1] = nullptr;

	tinyportals = 0;

	opaque = true;
	areaportal = false;

	cluster = -1;
	area = -1;
	occupied = -1;
}

// ------------------------------ Tree -----------------------------------

bspTree::bspTree()
{
	headnode = nullptr;
}