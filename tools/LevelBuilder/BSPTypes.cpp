#include "stdafx.h"
#include "BSPTypes.h"

// ------------------------------ Tris -----------------------------------

bspTris::bspTris()
{
	next = nullptr;
}

// ------------------------------ Face -----------------------------------

bspFace::bspFace()
{
	core::zero_this(this);
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