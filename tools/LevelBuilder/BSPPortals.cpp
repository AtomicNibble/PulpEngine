#include "stdafx.h"
#include "BSPTypes.h"
#include "LvlEntity.h"

#include "LvlFmts/mapFile/MapFile.h"

#include <IConsole.h>

namespace
{
    const float SIDESPACE = 8;

} // namespace

X_NAMESPACE_BEGIN(level)

// =============================================================================

void bspPortal::MakeNodePortal(XPlaneSet& planeSet, bspNode* node)
{
    auto w = node->getBaseWinding(planeSet);
    int32_t side;
    Planef plane;

    // clip the portal by all the other portals in the node
    for (auto* p = node->portals; p && w; p = p->next[side]) {
        if (p->nodes[Side::FRONT] == node) {
            side = 0;
            plane = p->plane;
        }
        else if (p->nodes[Side::BACK] == node) {
            side = 1;
            plane = -p->plane;
        }
        else {
            X_ERROR("BspPortal", "CutNodePortals_r: mislinked portal");
            side = 0; // quiet a compiler warning
        }

        if (!w->clip(plane, CLIP_EPSILON)) {
            w.reset();
        }
    }

    if (!w) {
        X_WARNING("BspPortal", "Winding is empty");
        return;
    }

#if X_DEBUG && 0
    w->Print();
#endif // X_DEBUG

    if (w->isTiny()) {
        return;
    }

    auto* new_portal = X_NEW(bspPortal, g_bspPortalArena, "Portal");
    new_portal->plane = planeSet[node->planenum];
    new_portal->onNode = node;
    new_portal->pWinding = w.release();
    new_portal->AddToNodes(node->children[Side::FRONT], node->children[Side::BACK]);
}

bool bspPortal::MakeTreePortals(XPlaneSet& planeSet, LvlEntity* pEnt)
{
    X_ASSERT_NOT_NULL(pEnt);
    X_ASSERT_NOT_NULL(pEnt->bspTree_.pHeadnode);

    MakeHeadnodePortals(pEnt->bspTree_);
    pEnt->bspTree_.pHeadnode->MakeTreePortals_r(planeSet);
    return true;
}

void bspPortal::MakeHeadnodePortals(bspTree& tree)
{
    auto* pNode = tree.pHeadnode;

    tree.outside_node.planenum = PLANENUM_LEAF;
    //		tree->outside_node.brushlist = NULL;
    tree.outside_node.portals = nullptr;
    tree.outside_node.opaque = false;

    // if no nodes, don't go any farther
    if (pNode->planenum == PLANENUM_LEAF) {
        return;
    }

    // pad with some space so there will never be null volume leafs
    AABB bounds;
    for (int32_t i = 0; i < 3; i++) {
        bounds.min[i] = tree.bounds.min[i] - SIDESPACE;
        bounds.max[i] = tree.bounds.max[i] + SIDESPACE;
        if (bounds.min[i] >= bounds.max[i]) {
            X_FATAL("BspPortal", "Backward tree volume");
        }
    }

    Planef bplanes[6];
    bspPortal* pPortals[6] = {};

    for (int32_t i = 0; i < 3; i++) {
        for (int32_t j = 0; j < 2; j++) {
            const int32_t n = j * 3 + i;

            auto* pPortal = X_NEW(bspPortal, g_bspPortalArena, "bspHeadPortal");
            pPortals[n] = pPortal;

            auto& pl = bplanes[n];

            if (j) {
                pl[i] = -1;
                pl.setDistance(-bounds.max[i]);
            }
            else {
                pl[i] = 1;
                pl.setDistance(bounds.min[i]);
            }

            pPortal->plane = pl;
            pPortal->pWinding = X_NEW(Winding, g_windingArena, "bspPortalWinding")(pl);
            pPortal->AddToNodes(pNode, &tree.outside_node);
        }
    }

    // clip the basewindings by all the other planes
    for (int32_t i = 0; i < 6; i++) {
        for (int32_t j = 0; j < 6; j++) {
            if (j == i) {
                continue;
            }
            if (!pPortals[i]->pWinding->clip(bplanes[j], ON_EPSILON)) {
                X_DELETE_AND_NULL(pPortals[i]->pWinding, g_windingArena);
            }
        }
    }

    core::ICVar* pLogVerbosity = gEnv->pConsole->getCVar("log_verbosity");
    if (!pLogVerbosity || pLogVerbosity->GetInteger() >= 1) {
        X_LOG1("BspPortal", "Head node windings");
        // print the head nodes portal bounds.
        for (int32_t i = 0; i < 6; i++) {
            pPortals[i]->pWinding->print();
        }
    }
}

void bspPortal::AddToNodes(bspNode* pFront, bspNode* pBack)
{
    X_ASSERT_NOT_NULL(this);

    if (nodes[Side::FRONT] || nodes[Side::BACK]) {
        X_ERROR("BspPortal", "Node already included");
    }

    nodes[Side::FRONT] = pFront;
    next[Side::FRONT] = pFront->portals;
    pFront->portals = this;

    nodes[Side::BACK] = pBack;
    next[Side::BACK] = pBack->portals;
    pBack->portals = this;
}

void bspPortal::RemoveFromNode(bspNode* pNode)
{
    bspPortal* pPortal = this;
    bspPortal **pp, *t;

    // remove reference to the current portal
    pp = &pNode->portals;

    while (1) {
        t = *pp;

        if (!t) {
            X_ERROR("BspPortal", "RemovePortalFromNode: portal not in leaf");
        }

        if (t == pPortal) {
            break;
        }

        if (t->nodes[Side::FRONT] == pNode) {
            pp = &t->next[Side::FRONT];
        }
        else if (t->nodes[Side::BACK] == pNode) {
            pp = &t->next[Side::BACK];
        }
        else {
            X_ERROR("BspPortal", "RemovePortalFromNode: portal not bounding leaf");
        }
    }

    if (pPortal->nodes[Side::FRONT] == pNode) {
        *pp = pPortal->next[Side::FRONT];
        pPortal->nodes[Side::FRONT] = nullptr;
    }
    else if (pPortal->nodes[Side::BACK] == pNode) {
        *pp = pPortal->next[Side::BACK];
        pPortal->nodes[Side::BACK] = nullptr;
    }
    else {
        X_ERROR("Portal", "RemovePortalFromNode: mislinked");
    }
}

const LvlBrushSide* bspPortal::FindAreaPortalSide(void) const
{
    // scan both bordering nodes brush lists for a portal brush
    // that shares the plane
    for (int32_t i = 0; i < Side::ENUM_COUNT; i++) {
        bspNode* node = nodes[i];
        node->brushes.size();
        for (size_t x = 0; x < node->brushes.size(); x++) {
            auto* pBrush = node->brushes[x];

            // do we have a side with a portal?
            if (!pBrush->combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL)) {
                continue;
            }

            auto* pOrigBrush = pBrush->pOriginal;

            // iterate the sides to find the portals.
            for (size_t j = 0; j < pOrigBrush->sides.size(); j++) {
                LvlBrushSide& side = pOrigBrush->sides[j];

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
                for (size_t k = 0; k < pBrush->sides.size(); k++) {
                    // skip self
                    if (k == j) {
                        continue;
                    }

                    auto& s2 = pOrigBrush->sides[k];
                    if (s2.pVisibleHull == nullptr) {
                        continue;
                    }

                    // portal side?
                    if (!s2.matInfo.getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
                        continue;
                    }

                    Vec3f center = s2.pVisibleHull->getCenter();

                    X_WARNING("BspPortal", "brush has multiple area portal sides at (%g,%g,%g)",
                        center[0], center[1], center[2]);

                    X_DELETE_AND_NULL(s2.pVisibleHull, g_windingArena);
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
        return false; // to global outsideleaf
    }

    if (nodes[Side::FRONT]->planenum != PLANENUM_LEAF || nodes[Side::BACK]->planenum != PLANENUM_LEAF) {
        X_ERROR("bspPortal", "not a leaf");
    }

    if (!nodes[Side::FRONT]->opaque && !nodes[Side::BACK]->opaque) {
        return true;
    }

    return false;
}

X_NAMESPACE_END