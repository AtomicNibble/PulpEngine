#include "stdafx.h"
#include "LevelBuilder.h"


namespace
{
	static const size_t BLOCK_SIZE = 1024;
		
}


int LvlBuilder::SelectSplitPlaneNum(bspNode* node, bspFace* faces)
{
	Vec3f halfSize = node->bounds.halfVec();
	size_t axis;

	float dist;
	Planef plane;
	int planeNum;

	for (axis = 0; axis < 3; axis++) 
	{
		if (halfSize[axis] > BLOCK_SIZE) 
		{
			float middleAxis = (node->bounds.min[axis] + halfSize[axis]);
			middleAxis /= BLOCK_SIZE;

			dist = BLOCK_SIZE * (math<float>::floor(middleAxis) + 1.0f);
		}
		else 
		{
			float minScaled = node->bounds.min[axis] / BLOCK_SIZE;

			dist = BLOCK_SIZE * (math<float>::floor(minScaled) + 1.0f);
		}

		if (dist > (node->bounds.min[axis] + 1.0f))
		{
			if (dist < (node->bounds.max[axis] - 1.0f))
			{
				plane[0] = plane[1] = plane[2] = 0.0f;
				plane[axis] = 1.0f;
				plane.setDistance(-dist);
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
	size_t bestValue;
	bspFace* bestSplit;
	bspFace* pFace, *pFace2;

	bestValue = -999999;
	havePortals = false;

	int32_t splits, facing, front, back;
	Planeside::Enum side;

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

		plane = planes[face.planenum];
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
			side = checkFace.w->PlaneSide(plane);
			if (side == Planeside::CROSS)
			{
				splits++;
			}
			else if (side == Planeside::BACK)
			{
				back++;
			}
			else if (side == Planeside::FRONT)
			{
				front++;
			}
		}

		size_t value = 5 * facing - 5 * splits;

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



void LvlBuilder::BuildFaceTree_r(bspNode* node, bspFace* faces)
{
	int splitPlaneNum = SelectSplitPlaneNum(node, faces);

	// if we don't have any more faces, this is a node
	if (splitPlaneNum == -1) {
		node->planenum = PLANENUM_LEAF;
		return;
	}

	Planeside::Enum side;

	XWinding* frontWinding;
	XWinding* backWinding;

	bspFace* childLists[2];
	bspFace* pNext, *pNewFace, *pFace;

	// partition the list
	node->planenum = splitPlaneNum;
	Planef& plane = planes[splitPlaneNum];
	childLists[0] = nullptr;
	childLists[1] = nullptr;

	for (pFace = faces; pFace; pFace = pNext)
	{
		pNext = pFace->pNext;

		bspFace& face = *pFace;

		if (face.planenum == node->planenum) 
		{
			X_DELETE(pFace, g_arena);
			continue;
		}

		side = face.w->PlaneSide(plane);
		if (side == Planeside::CROSS) 
		{
			face.w->Split(plane, 0.1f, &frontWinding, &backWinding);
			if (frontWinding) 
			{
				pNewFace = X_NEW(bspFace, g_arena, "bspFaceFrontWind");
				pNewFace->w = frontWinding;
				pNewFace->pNext = childLists[0];
				pNewFace->planenum = pFace->planenum;
				childLists[0] = pNewFace;
			}
			if (backWinding)
			{
				pNewFace = X_NEW(bspFace, g_arena, "bspFaceBackWind");
				pNewFace->w = backWinding;
				pNewFace->pNext = childLists[1];
				pNewFace->planenum = pFace->planenum;
				childLists[1] = pNewFace;
			}
			X_DELETE(pFace, g_arena);
		}
		else if (side == Planeside::FRONT)
		{
			pFace->pNext = childLists[0];
			childLists[0] = pFace;
		}
		else if (side == Planeside::BACK)
		{
			pFace->pNext = childLists[1];
			childLists[1] = pFace;
		}
	}

	size_t i;
	for (i = 0; i < 2; i++) {
		node->children[i] = X_NEW(bspNode, g_arena, "bspNode");
		node->children[i]->parent = node;
		node->children[i]->bounds = node->bounds;
	}

	// split the bounds if we have a nice axial plane
	for (i = 0; i < 3; i++) 
	{
		if (math<float>::abs(plane[i] - 1.f) < 0.001f) 
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

void LvlBuilder::FacesToBSP(LvlEntity& ent)
{
	X_LOG0("Bsp", "Building face list");

	bspTree& root = ent.bspTree;
	root.bounds.clear();
	root.headnode = X_NEW(bspNode, g_arena, "BspNode");
	
	bspFace* pFace = ent.bspFaces;
	for (; pFace; pFace = pFace->pNext)
	{
		const bspFace& face = *pFace;
		const XWinding& winding = *face.w;

		for (int32_t i = 0; i < winding.GetNumPoints(); i++)
		{
			root.bounds.add(winding[i].asVec3());
		}
	}

	// copy bounds.
	root.headnode->bounds = root.bounds;


	BuildFaceTree_r(root.headnode, ent.bspFaces);

}