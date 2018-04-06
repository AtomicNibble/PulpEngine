#pragma once

#include "LvlMaterial.h"

X_NAMESPACE_BEGIN(level)

struct BrushPlaneSide
{
    enum Enum
    {
        FRONT = 1,
        BACK,
        BOTH = FRONT | BACK, // same but yer ^^
        FACING
    };
};

struct LvlBrushSide
{
    LvlBrushSide();
    LvlBrushSide(const LvlBrushSide& oth);
    LvlBrushSide(LvlBrushSide&& oth);
    ~LvlBrushSide();

    LvlBrushSide& operator=(const LvlBrushSide& oth);
    LvlBrushSide& operator=(LvlBrushSide&& oth);

public:
    int32_t planenum;

    bool visible;
    bool culled;
    bool axial;
    bool __pad[1];

    LvlMaterial matInfo;

    Winding* pWinding;     // only clipped to the other sides of the brush
    Winding* pVisibleHull; // convex hull of all visible fragments
};

struct LvlBrush
{
    typedef core::Array<LvlBrushSide> SidesArr;

public:
    LvlBrush();
    LvlBrush(const LvlBrush& oth);
    LvlBrush(LvlBrush&& oth);
    ~LvlBrush();

    LvlBrush& operator=(const LvlBrush& oth);
    LvlBrush& operator=(LvlBrush&& oth);

    bool removeDuplicateBrushPlanes(void);
    bool createBrushWindings(const XPlaneSet& planes);
    bool boundBrush(const XPlaneSet& planes);
    bool calculateContents(void);
    bool isRectangle(void) const;
    float volume(const XPlaneSet& planes);

    BrushPlaneSide::Enum brushMostlyOnSide(const Planef& plane) const;

    size_t FilterBrushIntoTree_r(XPlaneSet& planes, bspNode* pNode);

    void Split(XPlaneSet& planes, int32_t planenum, LvlBrush** pFront, LvlBrush** pBack);
    void SplitMove(XPlaneSet& planes, int32_t planenum, LvlBrush** pFront, LvlBrush** pBack);

public:
    struct LvlBrush* pOriginal;

    AABB bounds;

    int32_t entityNum;
    int32_t brushNum;

    bool opaque;
    bool allsidesSameMat; // all the sides use same material.
    bool __pad[2];

    // the combined flags of all sides.
    // so if one side has portal type.
    // this will contain portal flag.
    engine::MaterialFlags combinedMatFlags;

    SidesArr sides;
};

X_NAMESPACE_END