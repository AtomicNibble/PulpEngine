#include "stdafx.h"
#include "LevelBuilder.h"




void LvlBuilder::FilterBrushesIntoTree(LvlEntity& ent)
{
	LvlBrush			*b, *newb;
	int					r;
	int					c_unique, c_clusters;

	X_LOG0("Lvl","----- FilterBrushesIntoTree -----");

	c_unique = 0;
	c_clusters = 0;

	LvlEntity::LvlBrushArr::Iterator it = ent.brushes.begin();
	for (; it != ent.brushes.end(); ++it)
	{
		c_unique++;

		b = it;

		newb = X_NEW(LvlBrush, g_arena, "BrushCopy")(*b);

		r = FilterBrushIntoTree_r(newb, ent.bspTree.headnode);

		c_clusters += r;
	}

	X_LOG0("Lvl", "%5i total brushes", c_unique);
	X_LOG0("Lvl", "%5i cluster references", c_clusters);
}

int LvlBuilder::FilterBrushIntoTree_r(LvlBrush* b, bspNode* node)
{
	LvlBrush		*front, *back;
	int				c;

	if (!b) {
		return 0;
	}

	// add it to the leaf list
	if (node->planenum == PLANENUM_LEAF) 
	{
	//	b->next = node->brushlist;
	//	node->brushlist = b;

		// classify the leaf by the structural brush
		if (b->opaque) {
			node->opaque = true;
		}

		return 1;
	}

	// split it by the node plane
	SplitBrush(b, node->planenum, &front, &back);
	X_DELETE(b, g_arena);

	c = 0;
	c += FilterBrushIntoTree_r(front, node->children[0]);
	c += FilterBrushIntoTree_r(back, node->children[1]);

	return c;
}

void LvlBuilder::SplitBrush(LvlBrush* brush, int32_t planenum,
	LvlBrush** front, LvlBrush** back)
{
	LvlBrush		*b[2];
	size_t			i, j;
	XWinding		*w, *cw[2], *midwinding;
	LvlBrushSide	*s, *cs;
	float			d, d_front, d_back;

	*front = *back = nullptr;
	Planef &plane = planes[planenum];

	// check all points
	d_front = d_back = 0;
	for (i = 0; i < brush->sides.size(); i++)
	{
		w = brush->sides[i].pWinding;
		if (!w) {
			continue;
		}
		for (j = 0; j < static_cast<size_t>(w->GetNumPoints()); j++) 
		{
			d = plane.distance((*w)[j].asVec3());
			if (d > 0 && d > d_front)
				d_front = d;
			if (d < 0 && d < d_back)
				d_back = d;
		}
	}
	if (d_front < 0.1)
	{	
		// only on back
		*back = brush;
		return;
	}
	if (d_back > -0.1) 
	{	
		// only on front
		*front = brush;
		return;
	}

	// create a new winding from the split plane
	w = X_NEW(XWinding, g_arena, "Winding")(plane);
	for (i = 0; i < brush->sides.size() && w; i++) {
		Planef &plane2 = planes[brush->sides[i].planenum ^ 1];
		w = w->Clip(plane2, 0); 
	}

	if (!w || w->IsTiny()) 
	{
		// the brush isn't really split
		BrushPlaneSide::Enum side;

		side = brush->BrushMostlyOnSide(plane);
		if (side == BrushPlaneSide::FRONT)
			*front = brush;
		if (side == BrushPlaneSide::BACK)
			*back = brush;
		return;
	}

	if (w->IsHuge()) {
		X_WARNING("LvlBrush", "SplitBrush: huge winding");
	}

	midwinding = w;

	// split it for real
	for (i = 0; i < 2; i++) {
		b[i] = X_NEW(LvlBrush,g_arena, "Brush")(*brush);
		b[i]->sides.clear();
	//	b[i]->next = NULL;
	//	b[i]->original = brush->original;
	}

	// split all the current windings
	for (i = 0; i < brush->sides.size(); i++) 
	{
		s = &brush->sides[i];
		w = s->pWinding;
		if (!w) {
			continue;
		}

		w->Split(plane, 0, &cw[0], &cw[1]);

		for (j = 0; j < 2; j++) 
		{
			if (!cw[j]) {
				continue;
			}

			cs = &b[j]->sides.AddOne();
			*cs = *s;
			cs->pWinding = cw[j];
		}
	}

	// see if we have valid polygons on both sides
	for (i = 0; i<2; i++)
	{
		if (!b[i]->boundBrush(planes)) {
			break;
		}

		if (b[i]->sides.size() < 3)
		{
			X_DELETE_AND_NULL(b[i], g_arena);
		}
	}


}