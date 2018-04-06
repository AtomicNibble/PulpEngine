#include "stdafx.h"
#include "FaceTreeBuilder.h"

#include "BSPTypes.h"

X_NAMESPACE_BEGIN(level)

FaceTreeBuilder::FaceTreeBuilder(XPlaneSet& planeset) :
    planeset_(planeset),
    numLeafs_(0)
{
}

size_t FaceTreeBuilder::getNumLeafs(void) const
{
    return numLeafs_;
}

bool FaceTreeBuilder::Build(bspNode* pRootNode, bspFace* pFaceList)
{
    X_ASSERT_NOT_NULL(pRootNode);
    X_ASSERT_NOT_NULL(pFaceList);

    BuildFaceTree_r(pRootNode, pFaceList);
    return true;
}

void FaceTreeBuilder::BuildFaceTree_r(bspNode* node, bspFace* faces)
{
    const int32_t splitPlaneNum = SelectSplitPlaneNum(node, faces);

    // if we don't have any more faces, this is a node
    if (splitPlaneNum == -1) {
        node->planenum = PLANENUM_LEAF;
        numLeafs_++;
        return;
    }

    Winding* frontWinding;
    Winding* backWinding;

    bspFace* childLists[Side::ENUM_COUNT];
    bspFace *pNext, *pNewFace, *pFace;

    // partition the list
    node->planenum = splitPlaneNum;
    const Planef& plane = planeset_[splitPlaneNum];
    childLists[Side::FRONT] = nullptr;
    childLists[Side::BACK] = nullptr;

    for (pFace = faces; pFace; pFace = pNext) {
        pNext = pFace->pNext;

        if (pFace->planenum == node->planenum) {
            X_DELETE(pFace, g_bspFaceArena);
            continue;
        }

        const auto side = pFace->w->planeSide(plane);
        if (side == PlaneSide::CROSS) {
            pFace->w->SplitMove(plane, CLIP_EPSILON * 2, &frontWinding, &backWinding, g_windingArena);
            if (frontWinding) {
                pNewFace = X_NEW(bspFace, g_bspFaceArena, "bspFaceFrontWind");
                pNewFace->w = frontWinding;
                pNewFace->pNext = childLists[Side::FRONT];
                pNewFace->planenum = pFace->planenum;
                childLists[Side::FRONT] = pNewFace;
            }
            if (backWinding) {
                pNewFace = X_NEW(bspFace, g_bspFaceArena, "bspFaceBackWind");
                pNewFace->w = backWinding;
                pNewFace->pNext = childLists[Side::BACK];
                pNewFace->planenum = pFace->planenum;
                childLists[Side::BACK] = pNewFace;
            }
            X_DELETE(pFace, g_bspFaceArena);
        }
        else if (side == PlaneSide::FRONT) {
            pFace->pNext = childLists[Side::FRONT];
            childLists[Side::FRONT] = pFace;
        }
        else if (side == PlaneSide::BACK) {
            pFace->pNext = childLists[Side::BACK];
            childLists[Side::BACK] = pFace;
        }
    }

    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        node->children[i] = X_NEW(bspNode, g_bspNodeArena, "bspNode");
        node->children[i]->parent = node;
        node->children[i]->bounds = node->bounds;
    }

    // split the bounds if we have a nice axial plane
    for (int32_t i = 0; i < 3; i++) {
        float val = math<float>::abs(plane[i] - 1.f);
        if (val < 0.001f) {
            node->children[Side::FRONT]->bounds.min[i] = plane.getDistance();
            node->children[Side::BACK]->bounds.max[i] = plane.getDistance();
            break;
        }
    }

    for (uint32_t i = 0; i < Side::ENUM_COUNT; i++) {
        BuildFaceTree_r(node->children[i], childLists[i]);
    }
}

int32_t FaceTreeBuilder::SelectSplitPlaneNum(bspNode* node, bspFace* faces)
{
    Vec3f halfSize = node->bounds.halfVec();

    // force split any bounds that cross block size boundry.
    // so if block size is 1024 anything crossing it we force a split on the boundry.
    for (int32_t axis = 0; axis < 3; axis++) {
        float blockBoundry;

        // if the bounds are over double boundry size split somewhere near the middle.
        // otherwise we split at first boundry after min.
        if (halfSize[axis] > BSP_TREE_BLOCK_SIZE) {
            float middleAxis = (node->bounds.min[axis] + halfSize[axis]);
            blockBoundry = middleAxis;
        }
        else {
            blockBoundry = node->bounds.min[axis];
        }

        // clamp it to nearest boundry.
        blockBoundry /= BSP_TREE_BLOCK_SIZE;
        blockBoundry = BSP_TREE_BLOCK_SIZE * (math<float>::floor(blockBoundry) + 1.0f);

        // does the boundry end inside the axis bounds?
        if (blockBoundry > (node->bounds.min[axis] + 1.0f)) {
            if (blockBoundry < (node->bounds.max[axis] - 1.0f)) {
                Planef plane;
                plane[axis] = 1.0f;
                plane.setDistance(blockBoundry);
                int32_t planeNum = FindFloatPlane(plane);
                return planeNum;
            }
        }
    }

    // pick one of the face planes
    // if we have any portal faces at all, only
    // select from them, otherwise select from
    // all faces
    bool havePortals;
    int32 bestValue;
    bspFace* bestSplit;
    bspFace *pFace, *pFace2;

    bestValue = -999999;
    havePortals = false;

    int32_t splits, facing, front, back;
    PlaneSide::Enum side;

    for (pFace = faces; pFace; pFace = pFace->pNext) {
        bspFace& face = *pFace;

        face.checked = false;
        if (face.portal) {
            havePortals = true;
            // don't break since we also reset checked.
        }
    }

    for (pFace = faces; pFace; pFace = pFace->pNext) {
        bspFace& face = *pFace;

        if (face.checked) {
            continue;
        }

        if (havePortals) {
            if (!face.portal) {
                continue;
            }
        }

        const auto& plane = planeset_[face.planenum];
        splits = 0;
        facing = 0;
        front = 0;
        back = 0;

        for (pFace2 = faces; pFace2; pFace2 = pFace2->pNext) {
            bspFace& checkFace = *pFace2;

            if (face.planenum == checkFace.planenum) {
                facing++;
                checkFace.checked = true; // won't need to test this plane again
                continue;
            }
            // get plane side.
            side = checkFace.w->planeSide(plane);
            if (side == PlaneSide::CROSS) {
                splits++;
            }
            else if (side == PlaneSide::BACK) {
                back++;
            }
            else if (side == PlaneSide::FRONT) {
                front++;
            }
        }

        // the best one is most facing and least
        // cross planes (splits)
        int32 value = (5 * facing) - (5 * splits);

        if (PlaneType::isTrueAxial(plane.getType())) {
            value += 5;
        }

        if (value > bestValue) {
            bestValue = value;
            bestSplit = pFace;
        }
    }

    if (bestValue == -999999) {
        return -1;
    }

    return bestSplit->planenum;
}

int32_t FaceTreeBuilder::FindFloatPlane(const Planef& plane)
{
    return planeset_.FindPlane(plane, PLANE_NORMAL_EPSILON, PLANE_DIST_EPSILON);
}

X_NAMESPACE_END
