#include "stdafx.h"
#include "FaceTreeBuilder.h"

#include "BSPTypes.h"

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
	int32_t splitPlaneNum = SelectSplitPlaneNum(node, faces);

	// if we don't have any more faces, this is a node
	if (splitPlaneNum == -1) {
		node->planenum = PLANENUM_LEAF;
		numLeafs_++;
		return;
	}

	size_t i;
	PlaneSide::Enum side;

	XWinding* frontWinding;
	XWinding* backWinding;

	bspFace* childLists[2];
	bspFace* pNext, *pNewFace, *pFace;

	// partition the list
	node->planenum = splitPlaneNum;
	Planef& plane = planeset_[splitPlaneNum];
	childLists[0] = nullptr;
	childLists[1] = nullptr;

	for (pFace = faces; pFace; pFace = pNext)
	{
		pNext = pFace->pNext;

		bspFace& face = *pFace;

		if (face.planenum == node->planenum)
		{
			X_DELETE(pFace, g_bspFaceArena);
			continue;
		}

		side = face.w->planeSide(plane);
		if (side == PlaneSide::CROSS)
		{
			face.w->Split(plane, CLIP_EPSILON * 2, &frontWinding, &backWinding, g_arena);
			if (frontWinding)
			{
				pNewFace = X_NEW(bspFace, g_bspFaceArena, "bspFaceFrontWind");
				pNewFace->w = frontWinding;
				pNewFace->pNext = childLists[0];
				pNewFace->planenum = pFace->planenum;
				childLists[0] = pNewFace;
			}
			if (backWinding)
			{
				pNewFace = X_NEW(bspFace, g_bspFaceArena, "bspFaceBackWind");
				pNewFace->w = backWinding;
				pNewFace->pNext = childLists[1];
				pNewFace->planenum = pFace->planenum;
				childLists[1] = pNewFace;
			}
			X_DELETE(pFace, g_bspFaceArena);
		}
		else if (side == PlaneSide::FRONT)
		{
			pFace->pNext = childLists[0];
			childLists[0] = pFace;
		}
		else if (side == PlaneSide::BACK)
		{
			pFace->pNext = childLists[1];
			childLists[1] = pFace;
		}
	}

	for (i = 0; i < 2; i++) {
		node->children[i] = X_NEW(bspNode, g_bspNodeArena, "bspNode");
		node->children[i]->parent = node;
		node->children[i]->bounds = node->bounds;
	}

	// split the bounds if we have a nice axial plane
	for (i = 0; i < 3; i++)
	{
		float val = math<float>::abs(plane[i] - 1.f);
		if (val < 0.001f)
		{
			node->children[0]->bounds.min[i] = plane.getDistance();
			node->children[1]->bounds.max[i] = plane.getDistance();
			break;
		}
	}

	for (i = 0; i < 2; i++) {
		BuildFaceTree_r(node->children[i], childLists[i]);
	}
}


int32_t FaceTreeBuilder::SelectSplitPlaneNum(bspNode* node, bspFace* faces)
{
	Vec3f halfSize = node->bounds.halfVec();
	size_t axis;

	float dist;
	Planef plane;
	int planeNum;

	for (axis = 0; axis < 3; axis++)
	{
		if (halfSize[axis] > BSP_TREE_BLOCK_SIZE)
		{
			// if the box is more than double the block size.
			// then get the middle of the axis and divide by block size
			// to work out how many blocks we can fit.
			float middleAxis = (node->bounds.min[axis] + halfSize[axis]);
			middleAxis /= BSP_TREE_BLOCK_SIZE;

			dist = BSP_TREE_BLOCK_SIZE * (math<float>::floor(middleAxis) + 1.0f);
		}
		else
		{
			// if two blocks don't fit inside the box.
			// then we see how many fits inside a half.
			// and round it up to atleast one.
			float minScaled = node->bounds.min[axis] / BSP_TREE_BLOCK_SIZE;

			dist = BSP_TREE_BLOCK_SIZE * (math<float>::floor(minScaled) + 1.0f);
		}

		// the resulting distance is always a multiple of BLOCK_SIZE
		// and atleast 1.

		// does the distance end inside the axis bounds?
		if (dist > (node->bounds.min[axis] + 1.0f))
		{
			if (dist < (node->bounds.max[axis] - 1.0f))
			{
				// create a plane on this axis with this distance
				plane[0] = plane[1] = plane[2] = 0.0f;
				plane[axis] = 1.0f;
				plane.setDistance(dist);
				planeNum = FindFloatPlane(plane);
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
	bspFace* pFace, *pFace2;

	bestValue = -999999;
	havePortals = false;

	int32_t splits, facing, front, back;
	PlaneSide::Enum side;

	for (pFace = faces; pFace; pFace = pFace->pNext)
	{
		bspFace& face = *pFace;

		face.checked = false;
		if (face.portal) {
			havePortals = true;
			// don't break since we also reset checked.
		}
	}

	for (pFace = faces; pFace; pFace = pFace->pNext)
	{
		bspFace& face = *pFace;

		if (face.checked) {
			continue;
		}

		if (havePortals) {
			if (!face.portal) {
				continue;
			}
		}

		plane = planeset_[face.planenum];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;

		for (pFace2 = faces; pFace2; pFace2 = pFace2->pNext)
		{
			bspFace& checkFace = *pFace2;

			if (face.planenum == checkFace.planenum)
			{
				facing++;
				checkFace.checked = true;	// won't need to test this plane again
				continue;
			}
			// get plane side.
			side = checkFace.w->planeSide(plane);
			if (side == PlaneSide::CROSS)
			{
				splits++;
			}
			else if (side == PlaneSide::BACK)
			{
				back++;
			}
			else if (side == PlaneSide::FRONT)
			{
				front++;
			}
		}

		// the best one is most facing and least
		// cross planes (splits)
		int32 value = 5 * facing - 5 * splits;

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