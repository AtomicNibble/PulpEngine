#include "stdafx.h"
#include "BSPTypes.h"
#include "LvlTypes.h"


bool LvlEntity::Process(XPlaneSet& planeSet)
{
	MakeStructuralFaceList();

	FacesToBSP();

	MakeTreePortals();

	FilterBrushesIntoTree();

	return true;
}

// ===================

bool LvlEntity::MakeStructuralFaceList(void)
{
	X_LOG0("LvlEnt", "MakeStructuralFaceList");
	X_ASSERT(bspFaces == nullptr, "bspFace already allocated")(bspFaces);

#if 1 // reverse toggle.
	size_t i, x;

	for (i = 0; i < brushes.size(); i++)
#else
	size_t x;
	int32_t i;

	for (i = ent.brushes.size() - 1; i >= 0; i--)
#endif
	{
		LvlBrush& brush = brushes[i];

		if (!brush.opaque)
		{
			// if it's not opaque and none of the sides are portals it can't be structual.
			if (!brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL))
			{
				continue;
			}
		}

		for (x = 0; x < brush.sides.size(); x++)
		{
			LvlBrushSide& side = brush.sides[x];

			if (!side.pWinding) {
				continue;
			}

			engine::MaterialFlags flags = side.matInfo.getFlags();

			// if combined flags are portal, check what this side is.
			if (brush.combinedMatFlags.IsSet(engine::MaterialFlag::PORTAL))
			{
				if (!flags.IsSet(engine::MaterialFlag::PORTAL))
				{
					continue;
				}
			}

			bspFace* pFace = X_NEW(bspFace, g_arena, "BspFace");
			pFace->planenum = side.planenum & ~1;
			pFace->w = side.pWinding->Copy();
			pFace->pNext = bspFaces;

			if (flags.IsSet(engine::MaterialFlag::PORTAL)) {
				pFace->portal = true;
			}

			bspFaces = pFace;
		}
	}

	return true;
}

bool LvlEntity::FacesToBSP(void)
{
	X_LOG0("LvlEntity", "Building face list");

	if (!this->bspFaces) {
		X_LOG0("LvlEntity", "Face list empty.");
		return false;
	}

	struct bspTree& root = this->bspTree;
	root.bounds.clear();
	root.headnode = X_NEW(bspNode, g_arena, "BspNode");

	size_t numLeafs = 0;
	size_t numFaces = 0;

	bspFace* pFace = bspFaces;
	for (; pFace; pFace = pFace->pNext)
	{
		numFaces++;

		const bspFace& face = *pFace;
		const XWinding& winding = *face.w;

		for (int32_t i = 0; i < winding.GetNumPoints(); i++)
		{
			root.bounds.add(winding[i].asVec3());
		}
	}

	X_LOG0("LvlEntity", "num faces: %i", numFaces);

	// copy bounds.
	root.headnode->bounds = root.bounds;

	// TODO
	//BuildFaceTree_r(root.headnode, bspFaces, numLeafs);

	// the have all been deleted
	bspFaces = nullptr;

	X_LOG0("LvlEntity", "num leafs: %i", numLeafs);
	return true;
}

bool LvlEntity::MakeTreePortals(void)
{
	return false; //  bspPortal::MakeTreePortals(this);
}

bool LvlEntity::FilterBrushesIntoTree(void)
{
	size_t		numUnique, numClusters;
	LvlBrush	*b, *newb;
	int			r;

	X_LOG0("LvlEntity", "----- FilterBrushesIntoTree -----");

	numUnique = 0;
	numClusters = 0;
	r = 0;

	LvlEntity::LvlBrushArr::Iterator it = brushes.begin();
	for (; it != brushes.end(); ++it)
	{
		numUnique++;

		b = it;

		newb = X_NEW(LvlBrush, g_arena, "BrushCopy")(*b);

		// r = LvlBrush::FilterBrushIntoTree_r(newb, bspTree.headnode);

		numClusters += r;
	}

	X_LOG0("LvlEntity", "%5i total brushes", numUnique);
	X_LOG0("LvlEntity", "%5i cluster references", numClusters);
	return true;
}


bool LvlEntity::FloodEntities(void)
{

	return true;
}

bool LvlEntity::PlaceOccupant(bspNode* node)
{

	return true;
}