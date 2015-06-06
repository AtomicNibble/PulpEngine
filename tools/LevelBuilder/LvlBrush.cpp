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

	front = nullptr;
	back = nullptr;

	if (!b) {
		return 0;
	}

	// add it to the leaf list
	if (node->planenum == PLANENUM_LEAF) 
	{
	//	b->next = node->brushlist;
	//	node->brushlist = b;
		node->brushes.append(b);

		// classify the leaf by the structural brush
		if (b->opaque) {
			node->opaque = true;
		}

		return 1;
	}

	// split it by the node plane
	SplitBrush(b, node->planenum, &front, &back);

	if (front && front->sides.size() < 6)
	{
		int goat = 0;
	}
	if (back && back->sides.size() < 6)
	{
		int goat = 0;
	}

	X_DELETE_AND_NULL(b, g_arena);

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

	if (brush->sides.size() < 6)
	{
		int goat = 0;
	}

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
		Planef &plane2 = planes[brush->sides[i].planenum ^ 1];
		w = w->Clip(plane2, 0); 
	}

	if (!w || w->IsTiny()) 
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

	if (w->IsHuge()) {
		X_WARNING("LvlBrush", "SplitBrush: huge winding");
		w->Print();
	}

	midwinding = w;

	static int goat = 0;
	goat++;

	if (goat == 15) {
		int xamel = 0;
	}

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

		cs->planenum = planenum ^ i ^ 1;
	//	cs->material = NULL;

		if (i == 0) {
			cs->pWinding = midwinding->Copy();
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
		w = w->Clip(plane2, 0);
	}

	if (!w || w->IsTiny())
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

	if (w->IsHuge()) {
		X_WARNING("LvlBrush", "SplitBrush: huge winding");
		w->Print();
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

		cs->planenum = planenum ^ i ^ 1;
		//	cs->material = NULL;

		if (i == 0) {
			cs->pWinding = midwinding->Copy();
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