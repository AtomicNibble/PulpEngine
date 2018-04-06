#pragma once

#ifndef X_BSP_TYPES_H_
#define X_BSP_TYPES_H_

#include <Containers\LinkedListIntrusive.h>

X_NAMESPACE_BEGIN(level)

X_DECLARE_ENUM(DrawSurfaceType)
(FACE, DECAL, PATCH);

struct LvlBrush;
struct LvlBrushSide;
struct LvlEntity;

struct FillStats
{
    FillStats();

    void print(void) const;

    size_t numOutside;
    size_t numInside;
    size_t numSolid;
};

struct bspFace
{
    bspFace();
    ~bspFace();

    struct bspFace* pNext;

    int planenum;
    bool portal;
    bool checked;
    bool _pad[2];

    Winding* w;
};

struct bspTree;
struct bspNode;

struct bspPortal
{
    bspPortal();
    ~bspPortal();

    static void MakeNodePortal(XPlaneSet& planeSet, bspNode* pNode);
    static bool MakeTreePortals(XPlaneSet& planeSet, LvlEntity* ent);

    // adds the portal to the nodes.
    void AddToNodes(bspNode* pFront, bspNode* pBack);
    void RemoveFromNode(bspNode* pNode);

    // looks for a side on the portal that is a area portal.
    const LvlBrushSide* FindAreaPortalSide(void) const;
    bool HasAreaPortalSide(void) const;
    bool PortalPassable(void) const;

private:
    static void MakeTreePortals_r(XPlaneSet& planeSet, bspNode* pNode);
    static void MakeHeadnodePortals(bspTree& tree);

public:
    Planef plane;
    bspNode* onNode; // nullptr = outside box.
    bspNode* nodes[Side::ENUM_COUNT];
    Winding* pWinding;
    bspPortal* next[Side::ENUM_COUNT];
};

struct bspNode
{
    friend struct bspTree;

    typedef core::Array<LvlBrush*> LvlBrushArr;

public:
    bspNode();
    ~bspNode();

    void CalcNodeBounds(void);
    core::UniquePointer<Winding> getBaseWinding(XPlaneSet& planeSet);
    void MakeTreePortals_r(XPlaneSet& planeSet);
    void FloodPortals_r(int32_t dist, size_t& floodedNum);

    void SplitPortals(XPlaneSet& planes);
    void FillOutside_r(FillStats& stats);
    void ClipSideByTree_r(XPlaneSet& planes, Winding* w, LvlBrushSide& side);

    // area number that the winding is in, or - 2 if it crosses multiple areas.
    int32_t CheckWindingInAreas_r(XPlaneSet& planes, const Winding* w);

    void FindAreas_r(size_t& numAreas);
    void FloodAreas_r(size_t areaNum, size_t& areaFloods);

    bool CheckAreas_r(void);

    int32_t PruneNodes_r(void);
    int32_t NumChildNodes(void);

    void FreeTreePortals_r(void);
    void FreeTree_r(void);

    void WriteNodes_r(XPlaneSet& planes, core::ByteStream& stream);

    X_INLINE bool AreaSet(void) const;
    X_INLINE bool IsLeaf(void) const;
    X_INLINE bool IsAreaLeaf(void) const;
    X_INLINE bool IsSolidLeaf(void) const;

public:
    // give each node a number.
    static int32_t NumberNodes_r(bspNode* pNode, int32_t nextNumber);
    static int32_t NumChildNodes_r(bspNode* pNode);

public:
    // leafs and nodes
    struct bspNode* parent;
    int32_t planenum; // -1 = leaf node
    AABB bounds;

    // nodes only
    int32_t nodeNumber; // set on save.
    struct bspNode* children[Side::ENUM_COUNT];

    // leafs only
    struct bspPortal* portals;

    bool opaque; // view can never be inside
    bool _pad[3];

    int32_t area;     // for areaportals
    int32_t occupied; // 1 or greater can reach entity

    LvlBrushArr brushes;

protected:
    void TreePrint_r(const XPlaneSet& planes, size_t depth) const;
};

struct bspTree
{
    bspTree();

    void Print(const XPlaneSet& planes) const;

public:
    bspNode* pHeadnode;
    bspNode outside_node;
    AABB bounds;
};

// good sizes currently.
X_ENSURE_LE(sizeof(bspPortal), 64, "Portal is large than cache line")
X_ENSURE_LE(sizeof(bspNode), 128, "Portal is large than two cache lines")

X_INLINE bool bspNode::AreaSet(void) const
{
    return area != -1;
}

X_INLINE bool bspNode::IsLeaf(void) const
{
    return planenum == PLANENUM_LEAF;
}

X_INLINE bool bspNode::IsAreaLeaf(void) const
{
    return IsLeaf() && AreaSet();
}

X_INLINE bool bspNode::IsSolidLeaf(void) const
{
    return IsLeaf() && !AreaSet();
}

X_NAMESPACE_END

#endif // X_BSP_TYPES_H_