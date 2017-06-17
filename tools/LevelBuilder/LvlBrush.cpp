#include "stdafx.h"
#include "LvlBuilder.h"





void LvlBuilder::SplitBrush(LvlBrush* brush, int32_t planenum,
	LvlBrush** front, LvlBrush** back)
{
	LvlBrush		*b[2];
	size_t			i, j;
	XWinding		*w, *cw[2], *midwinding;
	LvlBrushSide	*s, *cs;
	float			d, d_front, d_back;

	*front = *back = nullptr;
	Planef &plane = planes_[planenum];

	// check all points
	d_front = d_back = 0;
	for (i = 0; i < brush->sides.size(); i++)
	{
		w = brush->sides[i].pWinding;
		if (!w) {
			continue;
		}
		for (j = 0; j < static_cast<size_t>(w->getNumPoints()); j++) 
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
		*back = X_NEW(LvlBrush, g_arena, "BackBrush")(*brush);
		return;
	}
	if (d_back > -0.1) 
	{	
		// only on front
		*front = X_NEW(LvlBrush, g_arena, "FrontBrush")(*brush);
		return;
	}

	// create a new winding from the split plane
	w = X_NEW(XWinding, g_arena, "Winding")(plane);
	for (i = 0; i < brush->sides.size() && w; i++) {
		Planef &plane2 = planes_[brush->sides[i].planenum ^ 1];
		if (!w->clip(plane2, 0)) {
			X_DELETE_AND_NULL(w, g_arena);
		}
	}

	if (!w || w->isTiny()) 
	{
		// the brush isn't really split
		BrushPlaneSide::Enum side;

		side = brush->BrushMostlyOnSide(plane);
		if (side == BrushPlaneSide::FRONT)
			*front = X_NEW(LvlBrush, g_arena, "FrontBrush")(*brush);
		if (side == BrushPlaneSide::BACK)
			*back = X_NEW(LvlBrush, g_arena, "BackBrush")(*brush);
		return;
	}

	if (w->isHuge()) {
		X_WARNING("LvlBrush", "SplitBrush: huge winding");
		w->print();
	}

	midwinding = w;

	// split it for real
	for (i = 0; i < 2; i++) {
		b[i] = X_NEW(LvlBrush,g_arena, "Brush")(*brush);
		b[i]->sides.clear();
	//	b[i]->pOriginal = brush->pOriginal;
	}

	// split all the current windings
	for (i = 0; i < brush->sides.size(); i++) 
	{
		s = &brush->sides[i];
		w = s->pWinding;
		if (!w) {
			continue;
		}

		w->Split(plane, 0, &cw[0], &cw[1], g_arena);

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
		if (!b[i]->boundBrush(planes_)) {
			break;
		}

		if (b[i]->sides.size() < 3)
		{
			X_DELETE_AND_NULL(b[i], g_arena);
		}
	}

	if (!(b[0] && b[1]))
	{
		if (!b[0] && !b[1]) {
			X_LOG0("bspBrush", "split removed brush");
		}
		else {
			X_LOG0("bspBrush", "split not on both sides");
		}

		if (b[0])
		{
			X_DELETE_AND_NULL(b[0], g_arena);
			*front = X_NEW(LvlBrush, g_arena, "FrontBrush")(*brush);
		}
		if (b[1])
		{
			X_DELETE_AND_NULL(b[1], g_arena);
			*back = X_NEW(LvlBrush, g_arena, "BackBrush")(*brush);
		}
		return;
	}

	// add the midwinding to both sides
	for (i = 0; i<2; i++)
	{
		cs = &b[i]->sides.AddOne();

		cs->planenum = planenum ^ static_cast<int32>(i) ^ 1;
	//	cs->material = NULL;

		if (i == 0) {
			cs->pWinding = midwinding->Copy(g_arena);
		}
		else {
			cs->pWinding = midwinding;
		}
	}

	{
		float	v1;
		int		i;

		for (i = 0; i<2; i++)
		{
			v1 = b[i]->Volume(planes_);
			if (v1 < 1.0)
			{
				X_DELETE_AND_NULL(b[i], g_arena);
				b[i] = NULL;
				X_WARNING("LvlBrush", "SplitBrush: tiny volume after clip");
			}
		}
	}

	*front = b[0];
	*back = b[1];
}


// ===========================================

size_t LvlBrush::FilterBrushIntoTree_r(XPlaneSet& planes, bspNode* node)
{
	LvlBrush* pFront;
	LvlBrush* pBack;
	size_t count;

	pFront = nullptr;
	pBack = nullptr;
	count = 0;

	if (!this) {
		return 0;
	}

	// add it to the leaf list
	if (node->planenum == PLANENUM_LEAF)
	{
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
	this->Split(planes, node->planenum, pFront, pBack);

	X_DELETE(this, g_arena);

	if (pFront) {
		count += pFront->FilterBrushIntoTree_r(planes, node->children[0]);
	}
	if (pBack) {
		count += pBack->FilterBrushIntoTree_r(planes, node->children[1]);
	}

	return count;
}


void LvlBrush::Split(XPlaneSet& planes, int32_t planenum,
	LvlBrush*& front, LvlBrush*& back)
{
	LvlBrush		*b[2];
	size_t			i, j;
	XWinding		*w, *cw[2], *midwinding;
	LvlBrushSide	*s, *cs;
	float			d, d_front, d_back;

	front = back = nullptr;
	Planef &plane = planes[planenum];

	// check all points
	d_front = d_back = 0;
	for (i = 0; i < sides.size(); i++)
	{
		w = sides[i].pWinding;
		if (!w) {
			continue;
		}
		for (j = 0; j < static_cast<size_t>(w->getNumPoints()); j++)
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
		back = X_NEW(LvlBrush, g_arena, "BackBrush")(*this);
		return;
	}
	if (d_back > -0.1)
	{
		// only on front
		front = X_NEW(LvlBrush, g_arena, "FrontBrush")(*this);
		return;
	}

	// create a new winding from the split plane
	w = X_NEW(XWinding, g_arena, "Winding")(plane);
	for (i = 0; i < sides.size() && w; i++) 
	{
		Planef &plane2 = planes[sides[i].planenum ^ 1];
		if(!w->clip(plane2, 0)) {
			X_DELETE_AND_NULL(w, g_arena);
		}
	}

	if (!w || w->isTiny())
	{
		// the brush isn't really split
		BrushPlaneSide::Enum side;

		side = BrushMostlyOnSide(plane);
		if (side == BrushPlaneSide::FRONT)
			front = X_NEW(LvlBrush, g_arena, "FrontBrush")(*this);
		if (side == BrushPlaneSide::BACK)
			back = X_NEW(LvlBrush, g_arena, "BackBrush")(*this);
		return;
	}

	if (w->isHuge()) {
		X_WARNING("LvlBrush", "SplitBrush: huge winding");
		w->print();
	}

	midwinding = w;

	// split it for real
	for (i = 0; i < 2; i++) 
	{
		b[i] = X_NEW(LvlBrush, g_arena, "Brush")(*this);
		b[i]->sides.clear();
	}

	// split all the current windings
	for (i = 0; i < sides.size(); i++)
	{
		s = &sides[i];
		w = s->pWinding;
		if (!w) {
			continue;
		}

		w->Split(plane, 0, &cw[0], &cw[1], g_arena);

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

	if (!(b[0] && b[1]))
	{
		if (!b[0] && !b[1]) {
			X_LOG0("bspBrush", "split removed brush");
		}
		else {
			X_LOG0("bspBrush", "split not on both sides");
		}

		if (b[0])
		{
			X_DELETE_AND_NULL(b[0], g_arena);
			front = X_NEW(LvlBrush, g_arena, "FrontBrush")(*this);
		}
		if (b[1])
		{
			X_DELETE_AND_NULL(b[1], g_arena);
			back = X_NEW(LvlBrush, g_arena, "BackBrush")(*this);
		}
		return;
	}

	// add the midwinding to both sides
	for (i = 0; i<2; i++)
	{
		cs = &b[i]->sides.AddOne();

		cs->planenum = planenum ^ static_cast<int32_t>(i) ^ 1;
		//	cs->material = NULL;

		if (i == 0) {
			cs->pWinding = midwinding->Copy(g_arena);
		}
		else {
			cs->pWinding = midwinding;
		}
	}

	{
		float	v1;
		int		i;

		for (i = 0; i<2; i++)
		{
			v1 = b[i]->Volume(planes);
			if (v1 < 1.0)
			{
				X_DELETE_AND_NULL(b[i], g_arena);
				b[i] = NULL;
				X_WARNING("LvlBrush", "SplitBrush: tiny volume after clip");
			}
		}
	}

	front = b[0];
	back = b[1];
}