#include "stdafx.h"
#include "BSPTypes.h"

X_NAMESPACE_BEGIN(level)

FillStats::FillStats()
{
    core::zero_this(this);
}

void FillStats::print(void) const
{
    X_LOG0("FillStats", "^8%" PRIuS "^7 solid leafs", numSolid);
    X_LOG0("FillStats", "^8%" PRIuS "^7 leafs filled", numOutside);
    X_LOG0("FillStats", "^8%" PRIuS "^7 inside leafs", numInside);
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

bspFace::~bspFace()
{
    if (w) {
        X_DELETE(w, g_windingArena);
    }
}

// ------------------------------ Portal -----------------------------------

bspPortal::bspPortal()
{
    onNode = nullptr;
    core::zero_object(nodes);
    core::zero_object(next);
    pWinding = nullptr;
}

bspPortal::~bspPortal()
{
    if (pWinding) {
        X_DELETE(pWinding, g_windingArena);
    }
}

// ------------------------------ Node -----------------------------------

// ------------------------------ Tree -----------------------------------

bspTree::bspTree()
{
    pHeadnode = nullptr;
}

void bspTree::Print(const XPlaneSet& planes) const
{
    // print me baby!
    X_LOG0("BspTree", "Outside:");
    outside_node.TreePrint_r(planes, -1);

    X_LOG0("BspTree", "Printing tree:");
    if (pHeadnode) {
        size_t depth = 0;
        pHeadnode->TreePrint_r(planes, depth);
    }
}

X_NAMESPACE_END
