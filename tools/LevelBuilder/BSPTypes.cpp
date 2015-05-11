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
	core::zero_this(this);
}

// ------------------------------ Tree -----------------------------------

bspTree::bspTree()
{
	core::zero_this(this);
}