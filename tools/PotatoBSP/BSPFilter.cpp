#include "stdafx.h"
#include "BSPTypes.h"

namespace
{
	static const float PLANESIDE_EPSILON = 0.001f;

}



void BSPBuilder::SplitBrush(bspBrush* brush, int planenum, bspBrush **front, bspBrush **back)
{
	bspBrush	*b[2];
	int			i, j;
	XWinding	*w, *cw[2], *midwinding;
	BspSide		*s, *cs;
	float		d, d_front, d_back;

	b[0] = nullptr;
	b[1] = nullptr;

	*front = *back = nullptr;
	Planef &plane = planes[planenum];

	// check all points
	d_front = d_back = 0;
	for (i = 0; i < brush->numsides; i++)
	{
		w = brush->sides[i].pWinding;
		if (!w) {
			continue;
		}
		for (j = 0; j < w->GetNumPoints(); j++) {
			d = plane.distance((*w)[j]);
			if (d > 0 && d > d_front)
				d_front = d;
			if (d < 0 && d < d_back)
				d_back = d;
		}
	}
	if (d_front < 0.1) // PLANESIDE_EPSILON)
	{	// only on back
		*back = CopyBrush(brush);
		return;
	}
	if (d_back > -0.1) // PLANESIDE_EPSILON)
	{	// only on front
		*front = CopyBrush(brush);
		return;
	}

	// create a new winding from the split plane

	w = new XWinding(plane);
	for (i = 0; i < brush->numsides && w; i++) {
		Planef &plane2 = planes[brush->sides[i].planenum ^ 1];
		w = w->Clip(plane2, PLANESIDE_EPSILON);
	}

	if (!w || w->IsTiny()) {
		// the brush isn't really split
		int		side;

		side = brush->BrushMostlyOnSide(plane);
		if (side == BrushPlaneSide::FRONT)
			*front = CopyBrush(brush);
		if (side == BrushPlaneSide::BACK)
			*back = CopyBrush(brush);
		return;
	}

	if (w->IsHuge()) {
		X_WARNING("Brush", "huge winding");
	}

	midwinding = w;

	// split it for real

	for (i = 0; i < 2; i++) 
	{
		b[i] = AllocBrush(brush->numsides + 1);
		memcpy(b[i], brush, sizeof(bspBrush)-sizeof(brush->sides));
		b[i]->numsides = 0;
		b[i]->next = nullptr;
//		b[i]->original = brush->original;
	}

	// split all the current windings

	for (i = 0; i < brush->numsides; i++) {
		s = &brush->sides[i];
		w = s->pWinding;
		if (!w)
			continue;
		w->Split(plane, PLANESIDE_EPSILON, &cw[0], &cw[1]);
		for (j = 0; j < 2; j++) {
			if (!cw[j]) {
				continue;
			}

			/*	if ( cw[j]->IsTiny() )
			{
			delete cw[j];
			continue;
			}*/

			cs = &b[j]->sides[b[j]->numsides];
			b[j]->numsides++;
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

		if (b[i]->numsides < 3)
		{
			FreeBrush(b[i]);
			b[i] = nullptr;
		}
	}

	if (!(b[0] && b[1]))
	{
		if (!b[0] && !b[1])
			X_LOG0("Brush", "split removed brush");
		else
			X_LOG0("Brush", "split not on both sides");
		if (b[0])
		{
			FreeBrush(b[0]);
			*front = CopyBrush(brush);
		}
		if (b[1])
		{
			FreeBrush(b[1]);
			*back = CopyBrush(brush);
		}
		return;
	}

	// add the midwinding to both sides
	for (i = 0; i<2; i++)
	{
		cs = &b[i]->sides[b[i]->numsides];
		b[i]->numsides++;

		cs->planenum = planenum^i ^ 1;
		//	cs->material = NULL;
		if (i == 0)
			cs->pWinding = midwinding->Copy();
		else
			cs->pWinding = midwinding;
	}

	{
		float	v1;
		int		i;

		for (i = 0; i<2; i++)
		{
			v1 = b[i]->Volume(planes);
			if (v1 < 1.0)
			{
				FreeBrush(b[i]);
				b[i] = nullptr;
			}
		}
	}

	*front = b[0];
	*back = b[1];
}



int BSPBuilder::FilterBrushIntoTree_r(bspBrush* b, bspNode* node)
{
	bspBrush     *front, *back;
	int c;


	// dummy check 
	if (b == nullptr) {
		return 0;
	}

	// add it to the leaf list 
	if (node->planenum == PLANENUM_LEAF) 
	{
		// something somewhere is hammering brushlist 
		b->next = node->brushlist;
		node->brushlist = b;

		// classify the leaf by the structural brush 
		if (!b->detail) 
		{
			if (b->opaque) {
				node->opaque = true;
				node->areaportal = false;
			}
	//		else if (b->compileFlags & C_AREAPORTAL) {
	//			if (!node->opaque) {
	//				node->areaportal = true;
	//			}
	//		}
		}

		return 1;
	}

	// split it by the node plane 
	c = b->numsides;
	SplitBrush(b, node->planenum, &front, &back);
	FreeBrush(b);

	c = 0;
	c += FilterBrushIntoTree_r(front, node->children[0]);
	c += FilterBrushIntoTree_r(back, node->children[1]);

	return c;
}


void BSPBuilder::FilterStructuralBrushesIntoTree(const BspEntity& ent, bspTree* pTree)
{
	bspBrush         *b, *newb;
	int r;
	int c_unique, c_clusters;
	int i;

	X_LOG0("Bsp", "--- FilterStructuralBrushesIntoTree ---");

	c_unique = 0;
	c_clusters = 0;

	for (b = ent.pBrushes; b; b = b->next)
	{
		if (b->detail) {
			continue;
		}

		c_unique++;

		newb = CopyBrush(b);
		r = FilterBrushIntoTree_r(newb, pTree->headnode);

		c_clusters += r;

		// mark all sides as visible so drawsurfs are created
		if (r) {
			for (i = 0; i < b->numsides; i++) {
				if (b->sides[i].pWinding) {
					b->sides[i].visible = true;
				}
			}
		}
	}

	// emit some statistics 
	X_LOG0("Bsp", "%9d structural brushes", c_unique);
	X_LOG0("Bsp", "%9d cluster references", c_clusters);
}