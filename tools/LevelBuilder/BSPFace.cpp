#include "stdafx.h"
#include "LevelBuilder.h"


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



}