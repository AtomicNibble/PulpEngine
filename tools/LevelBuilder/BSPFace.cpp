#include "stdafx.h"
#include "LevelBuilder.h"


namespace
{
	static const size_t BLOCK_SIZE = 1024;
		
}


int LvlBuilder::SelectSplitPlaneNum(bspNode* node, LvlEntity::BspFaceArr& faces)
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

	bestValue = -999999;
	havePortals = false;


	LvlEntity::BspFaceArr::Iterator it2, it = faces.begin();

	for (; it != faces.end(); ++it)
	{
		bspFace& face = *it;

		face.checked = false;
		if (face.portal) {
			havePortals = true;
			// don't break since we also reset checked.
		}
	}

	for (; it != faces.end(); ++it)
	{
		bspFace& face = *it;

		if (face.checked) {
			continue;
		}

		if (havePortals) {
			if (!face.portal) {
				continue;
			}
		}

		plane = planes[face.planenum];


		int32_t splits, facing, front, back;
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;


		it2 = faces.begin();
		for (; it2 != faces.end(); ++it2)
		{
			bspFace& checkFace = *it;

			if (face.planenum == checkFace.planenum) 
			{
				facing++;
				checkFace.checked = true;	// won't need to test this plane again
				continue;
			}
			// get plane side.
			Planeside::Enum side = checkFace.w->PlaneSide(plane);
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
			bestSplit = &face;
		}

	}

	if (bestValue == -999999) {
		return -1;
	}

	return bestSplit->planenum;
}



void LvlBuilder::BuildFaceTree_r(bspNode* node, LvlEntity::BspFaceArr& faces)
{
	int splitPlaneNum = SelectSplitPlaneNum(node, faces);

	// if we don't have any more faces, this is a node
	if (splitPlaneNum == -1) {
		node->planenum = PLANENUM_LEAF;
		return;
	}

	bspFace* childLists[2];

	// partition the list
	node->planenum = splitPlaneNum;
	Planef& plane = planes[splitPlaneNum];
	childLists[0] = nullptr;
	childLists[1] = nullptr;


	LvlEntity::BspFaceArr::Iterator it2, it = faces.begin();
	for (; it != faces.end(); ++it)
	{
		bspFace& face = *it;

		if (face.planenum == node->planenum) 
		{
		//	FreeBspFace(split);
			continue;
		}

		Planeside::Enum side = face.w->PlaneSide(plane);
		if (side == Planeside::CROSS) 
		{
			XWinding* frontWinding;
			XWinding* backWinding;

			face.w->Split(plane, 0.1f, &frontWinding, &backWinding);
			if (frontWinding) {


			}
			if (backWinding) {


			}
		}
		else if (side == Planeside::FRONT)
		{
		//	side->next = childLists[0];
		//	childLists[0] = split;
		}
		else if (side == Planeside::BACK)
		{
		//	split->next = childLists[1];
		//	childLists[1] = split;
		}
	}
}

void LvlBuilder::FacesToBSP(LvlEntity& ent)
{
	X_LOG0("Bsp", "Building face list");

	bspTree& root = ent.bspTree;
	root.bounds.clear();
	root.headnode = X_NEW(bspNode, g_arena, "BspNode");
	
	// add the windows to the bounds.
	LvlEntity::BspFaceArr::ConstIterator it = ent.bspFaces.begin();
	LvlEntity::BspFaceArr::ConstIterator end = ent.bspFaces.end();
	for (; it != end; ++it)
	{
		const bspFace& face = *it;
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