#include "stdafx.h"
#include "LvlBuilder.h"

X_NAMESPACE_BEGIN(level)

// ==========================================

LvlBrushSide::LvlBrushSide() :
    planenum(0),
    visible(true),
    culled(false),
    axial(false),
    pWinding(nullptr),
    pVisibleHull(nullptr)
{
    core::zero_object(__pad);
}

LvlBrushSide::~LvlBrushSide()
{
    if (pWinding) {
        X_DELETE(pWinding, g_windingArena);
    }
    if (pVisibleHull) {
        X_DELETE(pVisibleHull, g_windingArena);
    }
}

LvlBrushSide::LvlBrushSide(const LvlBrushSide& oth) :
    matInfo(oth.matInfo),
    planenum(oth.planenum),
    visible(oth.visible),
    culled(oth.culled),
    axial(oth.axial),
    pWinding(nullptr),
    pVisibleHull(nullptr)
{
    if (oth.pWinding) {
        pWinding = oth.pWinding->Copy(g_windingArena);
    }
    if (oth.pVisibleHull) {
        pVisibleHull = oth.pVisibleHull->Copy(g_windingArena);
    }
}

LvlBrushSide::LvlBrushSide(LvlBrushSide&& oth) :
    matInfo(std::move(oth.matInfo)),
    planenum(oth.planenum),
    visible(oth.visible),
    culled(oth.culled),
    axial(oth.axial),
    pWinding(oth.pWinding),
    pVisibleHull(oth.pVisibleHull)
{
    oth.pWinding = nullptr;
    oth.pVisibleHull = nullptr;
}

LvlBrushSide& LvlBrushSide::operator=(const LvlBrushSide& oth)
{
    if (pWinding) {
        X_DELETE_AND_NULL(pWinding, g_windingArena);
    }
    if (pVisibleHull) {
        X_DELETE_AND_NULL(pVisibleHull, g_windingArena);
    }

    planenum = oth.planenum;
    visible = oth.visible;
    culled = oth.culled;
    axial = oth.axial;
    matInfo = oth.matInfo;

    if (oth.pWinding) {
        pWinding = oth.pWinding->Copy(g_windingArena);
    }
    if (oth.pVisibleHull) {
        pVisibleHull = oth.pVisibleHull->Copy(g_windingArena);
    }
    return *this;
}

LvlBrushSide& LvlBrushSide::operator=(LvlBrushSide&& oth)
{
    planenum = oth.planenum;
    visible = oth.visible;
    culled = oth.culled;
    axial = oth.axial;
    matInfo = std::move(oth.matInfo);

    pWinding = oth.pWinding;
    pVisibleHull = oth.pVisibleHull;

    oth.pWinding = nullptr;
    oth.pVisibleHull = nullptr;
    return *this;
}

// ==========================================

LvlBrush::LvlBrush() :
    pOriginal(nullptr),
    sides(g_arena)
{
    bounds.clear();
    entityNum = -1;
    brushNum = -1;

    opaque = false;
    allsidesSameMat = true;
}

LvlBrush::LvlBrush(const LvlBrush& oth) :
    sides(oth.sides)
{
    pOriginal = oth.pOriginal;

    bounds = oth.bounds;
    entityNum = oth.entityNum;
    brushNum = oth.brushNum;

    opaque = oth.opaque;
    allsidesSameMat = oth.allsidesSameMat;

    combinedMatFlags = oth.combinedMatFlags;
}

LvlBrush::LvlBrush(LvlBrush&& oth) :
    sides(std::move(oth.sides))
{
    pOriginal = oth.pOriginal;

    bounds = oth.bounds;
    entityNum = oth.entityNum;
    brushNum = oth.brushNum;

    opaque = oth.opaque;
    allsidesSameMat = oth.allsidesSameMat;

    combinedMatFlags = oth.combinedMatFlags;

    oth.bounds.clear();
    oth.entityNum = -1;
    oth.brushNum = -1;
}

LvlBrush& LvlBrush::operator=(const LvlBrush& oth)
{
    pOriginal = oth.pOriginal;

    bounds = oth.bounds;
    entityNum = oth.entityNum;
    brushNum = oth.brushNum;

    opaque = oth.opaque;
    allsidesSameMat = oth.allsidesSameMat;

    combinedMatFlags = oth.combinedMatFlags;

    sides = oth.sides;
    return *this;
}

LvlBrush& LvlBrush::operator=(LvlBrush&& oth)
{
    pOriginal = oth.pOriginal;

    bounds = oth.bounds;
    entityNum = oth.entityNum;
    brushNum = oth.brushNum;

    opaque = oth.opaque;
    allsidesSameMat = oth.allsidesSameMat;

    combinedMatFlags = oth.combinedMatFlags;

    sides = std::move(oth.sides);
    return *this;
}

LvlBrush::~LvlBrush()
{
}

bool LvlBrush::removeDuplicateBrushPlanes(void)
{
    for (size_t i = 1; i < sides.size(); i++) {
        LvlBrushSide& side = sides[i];

        // check for a degenerate plane
        if (side.planenum == -1) {
            X_WARNING("Brush", "Entity %" PRIi32 ", Brush %" PRIi32 ", Sides %" PRIuS ": degenerate plane(%" PRIuS ")",
                entityNum, brushNum, sides.size(), i);

            // remove it
            sides.removeIndex(i);

            i--;
            continue;
        }

        // check for duplication and mirroring
        for (size_t j = 0; j < i; j++) {
            if (side.planenum == sides[j].planenum) {
                X_WARNING("Brush", "Entity %" PRIi32 ", Brush %" PRIi32 ", Sides %" PRIuS ": duplicate plane(%" PRIuS ",%" PRIuS ")",
                    entityNum, brushNum, sides.size(), i, j);

                // remove the second duplicate
                sides.removeIndex(i);

                i--;
                break;
            }

            if (side.planenum == (sides[i].planenum ^ 1)) {
                // mirror plane, brush is invalid
                X_WARNING("Brush", "Entity %" PRIi32 ", Brush %" PRIi32 ", Sides %" PRIuS ": mirrored plane(%" PRIuS ",%" PRIuS ")",
                    entityNum, brushNum, sides.size(), i, j);
                return false;
            }
        }
    }
    return true;
}

bool LvlBrush::createBrushWindings(const XPlaneSet& planes)
{
    LvlBrushSide* pSide;

    for (size_t i = 0; i < sides.size(); i++) {
        pSide = &sides[i];
        const auto& plane = planes[pSide->planenum];
        auto* pWinding = X_NEW(Winding, g_windingArena, "BrushWinding")(plane);

        for (size_t j = 0; j < sides.size() && pWinding; j++) {
            if (i == j) {
                continue;
            }
            if (sides[j].planenum == (pSide->planenum ^ 1)) {
                continue; // back side clipaway
            }

            if (!pWinding->clip(planes[sides[j].planenum ^ 1], 0.01f)) {
                X_DELETE_AND_NULL(pWinding, g_windingArena);
            }
        }
        if (pSide->pWinding) {
            X_DELETE(pSide->pWinding, g_windingArena);
        }
        pSide->pWinding = pWinding;
    }

    return boundBrush(planes);
}

bool LvlBrush::boundBrush(const XPlaneSet& planes)
{
    bounds.clear();
    for (size_t i = 0; i < sides.size(); i++) {
        auto* pWinding = sides[i].pWinding;
        if (!pWinding) {
            continue;
        }
        for (size_t j = 0; j < pWinding->getNumPoints(); j++) {
            bounds.add((*pWinding)[j].asVec3());
        }
    }

    for (size_t i = 0; i < 3; i++) {
        if (bounds.min[i] < level::MIN_WORLD_COORD || bounds.max[i] > level::MAX_WORLD_COORD || bounds.min[i] >= bounds.max[i]) {
            // calculate a pos.
            Planef::Description Dsc;
            const Planef* pPlane = nullptr;
            if (sides.size() > 0) {
                pPlane = &planes[sides[0].planenum];
            }

            X_WARNING("LvlBrush", "Entity %i, Brush %i, Sides %i: failed "
                                  "to calculate brush bounds (%s)",
                entityNum, brushNum, sides.size(), pPlane ? pPlane->toString(Dsc) : "");
            return false;
        }
    }

    return true;
}

bool LvlBrush::calculateContents(void)
{
    if (sides.isEmpty()) {
        return false;
    }

    MaterialName matName = sides[0].matInfo.name;

    // a brush is only opaque if all sides are opaque
    opaque = true;
    allsidesSameMat = true;

    combinedMatFlags.Clear();

    for (const auto& side : sides) {
        if (!side.matInfo.pMaterial) {
            X_ERROR("Brush", "material not found for brush side. ent: %i brush: %i",
                entityNum, brushNum);
            return false;
        }

        engine::Material* pMat = side.matInfo.pMaterial;
        const auto flags = pMat->getFlags();

        combinedMatFlags |= flags;

        if (flags.IsSet(engine::MaterialFlag::PORTAL)) {
            opaque = false;
        }
        else if (pMat->getCoverage() != engine::MaterialCoverage::OPAQUE) {
            opaque = false;
        }

        if (matName != side.matInfo.name) {
            allsidesSameMat = false;
        }
    }

    return true;
}

bool LvlBrush::isRectangle(void) const
{
    if (sides.size() != 6) {
        return false;
    }

    // we want to know if all the planes are axial.
    for (auto& side : sides) {
        if (!side.axial) {
            return false;
        }
    }

    return true;
}

float LvlBrush::volume(const XPlaneSet& planes)
{
    // grab the first valid point as the corner
    size_t i;
    Winding* w = nullptr;
    for (i = 0; i < sides.size(); i++) {
        w = sides[i].pWinding;
        if (w) {
            break;
        }
    }
    if (!w) {
        return 0.f;
    }

    const Vec3f& corner = (*w)[0].asVec3();

    // make tetrahedrons to all other faces
    float volume = 0;
    for (; i < sides.size(); i++) {
        w = sides[i].pWinding;
        if (!w) {
            continue;
        }
        auto& plane = planes[sides[i].planenum];
        float d = -plane.distance(corner);
        float area = w->getArea();
        volume += d * area;
    }

    volume /= 3;
    return volume;
}

BrushPlaneSide::Enum LvlBrush::brushMostlyOnSide(const Planef& plane) const
{
    float max = 0;
    BrushPlaneSide::Enum side = BrushPlaneSide::FRONT;
    for (size_t i = 0; i < sides.size(); i++) {
        auto* w = sides[i].pWinding;
        if (!w) {
            continue;
        }

        for (size_t j = 0; j < w->getNumPoints(); j++) {
            float d = plane.distance((*w)[j].asVec3());
            if (d > max) {
                max = d;
                side = BrushPlaneSide::FRONT;
            }
            if (-d > max) {
                max = -d;
                side = BrushPlaneSide::BACK;
            }
        }
    }
    return side;
}

size_t LvlBrush::FilterBrushIntoTree_r(XPlaneSet& planes, bspNode* node)
{
    if (!this) {
        return 0;
    }

    // add it to the leaf list
    if (node->planenum == PLANENUM_LEAF) {
        //	b->next = node->brushlist;
        //	node->brushlist = b;
        node->brushes.append(this);

        // classify the leaf by the structural brush
        if (this->opaque) {
            node->opaque = true;
        }

        return 1;
    }

    // split it by the node plane
    LvlBrush *pFront, *pBack;
    SplitMove(planes, node->planenum, &pFront, &pBack);

    X_DELETE(this, g_arena);

    size_t count = 0;

    if (pFront) {
        count += pFront->FilterBrushIntoTree_r(planes, node->children[Side::FRONT]);
    }
    if (pBack) {
        count += pBack->FilterBrushIntoTree_r(planes, node->children[Side::BACK]);
    }

    return count;
}

void LvlBrush::Split(XPlaneSet& planes, int32_t planenum, LvlBrush** pFront, LvlBrush** pBack)
{
    *pFront = *pBack = nullptr;

    Winding* w = nullptr;
    const Planef& plane = planes[planenum];

    // check all points
    float d_front = 0.f;
    float d_back = 0.f;
    for (size_t i = 0; i < sides.size(); i++) {
        w = sides[i].pWinding;
        if (!w) {
            continue;
        }
        for (size_t j = 0; j < w->getNumPoints(); j++) {
            float d = plane.distance((*w)[j].asVec3());
            if (d > 0 && d > d_front) {
                d_front = d;
            }
            if (d < 0 && d < d_back) {
                d_back = d;
            }
        }
    }
    if (d_front < 0.1) {
        // only on back
        *pBack = X_NEW(LvlBrush, g_arena, "BackBrush")(*this);
        return;
    }
    if (d_back > -0.1) {
        // only on front
        *pFront = X_NEW(LvlBrush, g_arena, "FrontBrush")(*this);
        return;
    }

    // create a new winding from the split plane
    w = X_NEW(Winding, g_windingArena, "Winding")(plane);
    for (size_t i = 0; i < sides.size() && w; i++) {
        Planef& plane2 = planes[sides[i].planenum ^ 1];
        if (!w->clip(plane2, 0)) {
            X_DELETE_AND_NULL(w, g_windingArena);
        }
    }

    if (!w || w->isTiny()) {
        // the brush isn't really split
        auto side = brushMostlyOnSide(plane);
        if (side == BrushPlaneSide::FRONT) {
            *pFront = X_NEW(LvlBrush, g_arena, "FrontBrush")(*this);
        }
        if (side == BrushPlaneSide::BACK) {
            *pBack = X_NEW(LvlBrush, g_arena, "BackBrush")(*this);
        }
        return;
    }

    if (w->isHuge()) {
        X_WARNING("LvlBrush", "Split: huge winding");
        w->print();
    }

    auto* pMidwinding = w;

    // split it for real
    LvlBrush* pBrushes[Side::ENUM_COUNT];
    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        pBrushes[i] = X_NEW(LvlBrush, g_arena, "Brush")(*this);
        pBrushes[i]->sides.clear();
    }

    // split all the current windings
    for (size_t i = 0; i < sides.size(); i++) {
        auto& s = sides[i];
        w = s.pWinding;
        if (!w) {
            continue;
        }

        Winding* cw[Side::ENUM_COUNT];

        w->Split(plane, 0, &cw[Side::FRONT], &cw[Side::BACK], g_windingArena);

        for (uint32_t j = 0; j < Side::ENUM_COUNT; j++) {
            if (!cw[j]) {
                continue;
            }

            auto& cs = pBrushes[j]->sides.AddOne();
            cs = s;
            cs.pWinding = cw[j];
        }
    }

    // see if we have valid polygons on both sides
    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        if (!pBrushes[i]->boundBrush(planes)) {
            break;
        }

        if (pBrushes[i]->sides.size() < 3) {
            X_DELETE_AND_NULL(pBrushes[i], g_arena);
        }
    }

    if (!(pBrushes[Side::FRONT] && pBrushes[Side::FRONT])) {
        if (!pBrushes[Side::FRONT] && !pBrushes[Side::BACK]) {
            X_LOG0("LvlBrush", "split removed brush");
        }
        else {
            X_LOG0("LvlBrush", "split not on both sides");
        }

        if (pBrushes[Side::FRONT]) {
            X_DELETE_AND_NULL(pBrushes[Side::FRONT], g_arena);
            *pFront = X_NEW(LvlBrush, g_arena, "FrontBrush")(*this);
        }
        if (pBrushes[Side::BACK]) {
            X_DELETE_AND_NULL(pBrushes[Side::BACK], g_arena);
            *pBack = X_NEW(LvlBrush, g_arena, "BackBrush")(*this);
        }
        return;
    }

    // add the midwinding to both sides
    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        auto& cs = pBrushes[i]->sides.AddOne();
        cs.planenum = planenum ^ static_cast<int32_t>(i) ^ 1;
        //	cs->material = NULL;

        if (i == 0) {
            cs.pWinding = pMidwinding->Copy(g_windingArena);
        }
        else {
            cs.pWinding = pMidwinding;
        }
    }

    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        float v1 = pBrushes[i]->volume(planes);
        if (v1 < 1.0) {
            X_DELETE_AND_NULL(pBrushes[i], g_arena);
            X_WARNING("LvlBrush", "SplitBrush: tiny volume after clip");
        }
    }

    *pFront = pBrushes[Side::FRONT];
    *pBack = pBrushes[Side::BACK];
}

void LvlBrush::SplitMove(XPlaneSet& planes, int32_t planenum, LvlBrush** pFront, LvlBrush** pBack)
{
    *pFront = *pBack = nullptr;

    Winding* w = nullptr;
    const Planef& plane = planes[planenum];

    // check all points
    float d_front = 0.f;
    float d_back = 0.f;
    for (size_t i = 0; i < sides.size(); i++) {
        w = sides[i].pWinding;
        if (!w) {
            continue;
        }
        for (size_t j = 0; j < w->getNumPoints(); j++) {
            float d = plane.distance((*w)[j].asVec3());
            if (d > 0 && d > d_front) {
                d_front = d;
            }
            if (d < 0 && d < d_back) {
                d_back = d;
            }
        }
    }
    if (d_front < 0.1) {
        // only on back
        *pBack = X_NEW(LvlBrush, g_arena, "BackBrush")(std::move(*this));
        return;
    }
    if (d_back > -0.1) {
        // only on front
        *pFront = X_NEW(LvlBrush, g_arena, "FrontBrush")(std::move(*this));
        return;
    }

    // create a new winding from the split plane
    w = X_NEW(Winding, g_windingArena, "Winding")(plane);
    for (size_t i = 0; i < sides.size() && w; i++) {
        Planef& plane2 = planes[sides[i].planenum ^ 1];
        if (!w->clip(plane2, 0)) {
            X_DELETE_AND_NULL(w, g_windingArena);
        }
    }

    if (!w || w->isTiny()) {
        // the brush isn't really split
        auto side = brushMostlyOnSide(plane);
        if (side == BrushPlaneSide::FRONT) {
            *pFront = X_NEW(LvlBrush, g_arena, "FrontBrush")(std::move(*this));
        }
        if (side == BrushPlaneSide::BACK) {
            *pBack = X_NEW(LvlBrush, g_arena, "BackBrush")(std::move(*this));
        }
        return;
    }

    if (w->isHuge()) {
        X_WARNING("LvlBrush", "Split: huge winding");
        w->print();
    }

    auto* pMidwinding = w;

    // split it for real
    LvlBrush* pBrushes[Side::ENUM_COUNT];
    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        pBrushes[i] = X_NEW(LvlBrush, g_arena, "Brush")(*this);
        pBrushes[i]->sides.clear();
    }

    // split all the current windings
    for (size_t i = 0; i < sides.size(); i++) {
        auto& s = sides[i];
        w = s.pWinding;
        if (!w) {
            continue;
        }

        Winding* cw[Side::ENUM_COUNT];

        w->Split(plane, 0, &cw[Side::FRONT], &cw[Side::BACK], g_windingArena);

        for (uint32_t j = 0; j < Side::ENUM_COUNT; j++) {
            if (!cw[j]) {
                continue;
            }

            auto& cs = pBrushes[j]->sides.AddOne();
            cs = s;
            cs.pWinding = cw[j];
        }
    }

    // see if we have valid polygons on both sides
    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        if (!pBrushes[i]->boundBrush(planes)) {
            break;
        }

        if (pBrushes[i]->sides.size() < 3) {
            X_DELETE_AND_NULL(pBrushes[i], g_arena);
        }
    }

    if (!(pBrushes[Side::FRONT] && pBrushes[Side::FRONT])) {
        if (!pBrushes[Side::FRONT] && !pBrushes[Side::BACK]) {
            X_LOG0("LvlBrush", "split removed brush");
        }
        else {
            X_LOG0("LvlBrush", "split not on both sides");
        }

        if (pBrushes[Side::FRONT]) {
            X_DELETE_AND_NULL(pBrushes[Side::FRONT], g_arena);
            *pFront = X_NEW(LvlBrush, g_arena, "FrontBrush")(std::move(*this));
        }
        if (pBrushes[Side::BACK]) {
            X_DELETE_AND_NULL(pBrushes[Side::BACK], g_arena);
            *pBack = X_NEW(LvlBrush, g_arena, "BackBrush")(std::move(*this));
        }
        return;
    }

    // add the midwinding to both sides
    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        auto& cs = pBrushes[i]->sides.AddOne();
        cs.planenum = planenum ^ static_cast<int32_t>(i) ^ 1;
        //	cs->material = NULL;

        if (i == 0) {
            cs.pWinding = pMidwinding->Copy(g_windingArena);
        }
        else {
            cs.pWinding = pMidwinding;
        }
    }

    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        float v1 = pBrushes[i]->volume(planes);
        if (v1 < 1.0) {
            X_DELETE_AND_NULL(pBrushes[i], g_arena);
            X_WARNING("LvlBrush", "SplitBrush: tiny volume after clip");
        }
    }

    *pFront = pBrushes[Side::FRONT];
    *pBack = pBrushes[Side::BACK];
}

X_NAMESPACE_END