#include "stdafx.h"

#include <ITimer.h>
#include <IProfile.h>

#include "BSPTypes.h"
#include "MapTypes.h"

#if 0

#define	PSIDE_FRONT			1
#define	PSIDE_BACK			2
#define	PSIDE_BOTH			(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING		4

#define	PLANESIDE_EPSILON	0.001f


int BrushSizeForSides(int numsides) 
{
	int		c;

	c = sizeof(uBrush_t)+sizeof(side_t)* (numsides - 6);

	return c;
}


int BrushMostlyOnSide(uBrush_t *brush, Planef &plane) 
{
	int			i, j;
	XWinding	*w;
	float		d, max;
	int			side;

	max = 0;
	side = PSIDE_FRONT;
	for (i = 0; i < brush->numsides; i++) {
		w = brush->sides[i].winding;
		if (!w)
			continue;
		for (j = 0; j < w->GetNumPoints(); j++)
		{
			d = plane.distance((*w)[j]);
			if (d > max)
			{
				max = d;
				side = PSIDE_FRONT;
			}
			if (-d > max)
			{
				max = -d;
				side = PSIDE_BACK;
			}
		}
	}
	return side;
}


uBrush_t* CopyBrush(uBrush_t *brush)
{
	uBrush_t *newbrush;
	int			size;
	int			i;

	size = BrushSizeForSides(brush->numsides);

	newbrush = AllocBrush(brush->numsides);
	memcpy(newbrush, brush, size);

	for (i = 0; i<brush->numsides; i++)
	{
		if (brush->sides[i].winding)
			newbrush->sides[i].winding = brush->sides[i].winding->Copy();
	}

	return newbrush;
}

float BSPData::BrushVolume(uBrush_t *brush)
{
	int			i;
	XWinding	*w;
	Vec3f		corner;
	float		d, area, volume;
	Planef		*plane;

	if (!brush)
		return 0;

	// grab the first valid point as the corner

	w = NULL;
	for (i = 0; i < brush->numsides; i++) {
		w = brush->sides[i].winding;
		if (w)
			break;
	}
	if (!w) {
		return 0;
	}

	corner = (*w)[0];

	// make tetrahedrons to all other faces

	volume = 0;
	for (; i < brush->numsides; i++)
	{
		w = brush->sides[i].winding;
		if (!w)
			continue;
		plane = &planes[brush->sides[i].planenum];
		d = -plane->distance(corner);
		area = w->GetArea();
		volume += d * area;
	}

	volume /= 3;
	return volume;
}


/*
================
SplitBrush

Generates two new brushes, leaving the original
unchanged
================
*/
void BSPData::SplitBrush(uBrush_t *brush, int planenum, uBrush_t **front, uBrush_t **back)
{
	uBrush_t	*b[2];
	int			i, j;
	XWinding	*w, *cw[2], *midwinding;
	side_t		*s, *cs;
	float		d, d_front, d_back;

	b[0] = nullptr;
	b[1] = nullptr;

	*front = *back = NULL;
	Planef &plane = planes[planenum];

	// check all points
	d_front = d_back = 0;
	for (i = 0; i < brush->numsides; i++)
	{
		w = brush->sides[i].winding;
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

		side = BrushMostlyOnSide(brush, plane);
		if (side == PSIDE_FRONT)
			*front = CopyBrush(brush);
		if (side == PSIDE_BACK)
			*back = CopyBrush(brush);
		return;
	}

	if (w->IsHuge()) {
		X_WARNING("Brush", "huge winding");
	}

	midwinding = w;

	// split it for real

	for (i = 0; i < 2; i++) {
		b[i] = AllocBrush(brush->numsides + 1);
		memcpy(b[i], brush, sizeof(uBrush_t)-sizeof(brush->sides));
		b[i]->numsides = 0;
		b[i]->next = NULL;
		b[i]->original = brush->original;
	}

	// split all the current windings

	for (i = 0; i < brush->numsides; i++) {
		s = &brush->sides[i];
		w = s->winding;
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
			cs->winding = cw[j];
		}
	}


	// see if we have valid polygons on both sides

	for (i = 0; i<2; i++)
	{
		if (!BoundBrush(b[i])) {
			break;
		}

		if (b[i]->numsides < 3)
		{
			FreeBrush(b[i]);
			b[i] = NULL;
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
			cs->winding = midwinding->Copy();
		else
			cs->winding = midwinding;
	}

	{
		float	v1;
		int		i;

		for (i = 0; i<2; i++)
		{
			v1 = BrushVolume(b[i]);
			if (v1 < 1.0)
			{
				FreeBrush(b[i]);
				b[i] = NULL;
				//			common->Printf ("tiny volume after clip\n");
			}
		}
	}

	*front = b[0];
	*back = b[1];
}



int BSPData::FilterBrushIntoTree_r(uBrush_t *b, node_t *node)
{
	uBrush_t		*front, *back;
	int				c;

	if (!b) {
		return 0;
	}

	// add it to the leaf list
	if (node->planenum == PLANENUM_LEAF) {
		b->next = node->brushlist;
		node->brushlist = b;

		// classify the leaf by the structural brush
		if (b->opaque) {
			node->opaque = true;
		}

		return 1;
	}

	// split it by the node plane
	SplitBrush(b, node->planenum, &front, &back);
	FreeBrush(b);

	c = 0;
	c += FilterBrushIntoTree_r(front, node->children[0]);
	c += FilterBrushIntoTree_r(back, node->children[1]);

	return c;
}


/*
=====================
FilterBrushesIntoTree

Mark the leafs as opaque and areaportals and put brush
fragments in each leaf so portal surfaces can be matched
to materials
=====================
*/
void BSPData::FilterBrushesIntoTree(uEntity_t *e)
{
	primitive_t			*prim;
	uBrush_t			*b, *newb;
	int					r;
	int					c_unique, c_clusters;

	X_LOG0("Brush", "----- FilterBrushesIntoTree -----");

	c_unique = 0;
	c_clusters = 0;
	for (prim = e->primitives; prim; prim = prim->next) {
		b = prim->brush;
		if (!b) {
			continue;
		}
		c_unique++;
		newb = CopyBrush(b);
		r = FilterBrushIntoTree_r(newb, e->tree->headnode);
		c_clusters += r;
	}

	X_LOG0("Brush", "%5i total brushes", c_unique);
	X_LOG0("Brush", "%5i cluster references", c_clusters);

}


#endif