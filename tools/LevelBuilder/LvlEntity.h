#pragma once

#include "BSPTypes.h"
#include "LvlBrush.h"

X_NAMESPACE_BEGIN(level)

namespace mapFile
{
    class XMapFile;
    class XMapEntity;
    class XMapBrush;
    class XMapPatch;
} // namespace mapFile

struct LvlStats
{
    LvlStats()
    {
        core::zero_this(this);
    }
    int32_t numEntities;
    int32_t numPatches;
    int32_t numBrushes;
    int32_t numAreaPortals;
    int32_t numFaceLeafs;
};

struct LvlInterPortal
{
    LvlInterPortal();

    int32_t area0;
    int32_t area1;
    const LvlBrushSide* pSide;
};

struct LvlEntity
{
    typedef core::Array<LvlBrush> LvlBrushArr;
    typedef core::Array<LvlTris> TrisArr;
    typedef core::Array<LvlInterPortal> LvlInterPortalArr;
    typedef core::Array<LvlEntity> LvlEntsArr;

public:
    LvlEntity();
    ~LvlEntity();

    bool FindInterAreaPortals(void);
    bool FindInterAreaPortals_r(bspNode* node);

    bool MakeStructuralFaceList(void);
    bool FacesToBSP(XPlaneSet& planeSet);
    bool MakeTreePortals(XPlaneSet& planeSet);
    bool FilterBrushesIntoTree(XPlaneSet& planeSet);
    bool FloodEntities(XPlaneSet& planeSet, LvlEntsArr& ents);
    bool FillOutside(void);
    bool ClipSidesByTree(XPlaneSet& planeSet);
    bool FloodAreas(void);
    bool PruneNodes(void);

    bool AddMapTriToAreas(XPlaneSet& planeSet, const LvlTris& tris);

private:
    bool PlaceOccupant(XPlaneSet& planeSet, bspNode* node, size_t& floodedNum);

    void ClipSideByTree_r(Winding* w, LvlBrushSide& side, bspNode* node);
    void FindAreas_r(bspNode* node, size_t& numAreas);

    static bool CheckAreas_r(bspNode* pNode);

public:
    Vec3f origin;
    Vec3f angle; // euelr
    AABB bounds; // set for models, only currently.

    LvlBrushArr brushes;
    TrisArr patches;
    LvlInterPortalArr interPortals;
    // bsp data.
    bspFace* pBspFaces;
    bspTree bspTree_;

    size_t numAreas;

    game::ClassType::Enum classType;
    KeyPair epairs;
};

X_NAMESPACE_END