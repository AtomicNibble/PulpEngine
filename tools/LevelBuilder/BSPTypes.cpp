#include "stdafx.h"
#include "BSPTypes.h"

bspBrush::bspBrush()
{
	core::zero_this(this);
}

bspBrush::bspBrush(const bspBrush& oth)
{

	next = oth.next;

	// used for poviding helpful error msg's
	entityNum = oth.entityNum;
	brushNum = oth.brushNum;

	bounds = oth.bounds;
	opaque = oth.opaque;
	detail = oth.detail;

	numsides = oth.numsides;

	// copy windinds.
	for (int i = 0; i < numsides; i++)
	{
		sides[i] = oth.sides[i];

		if (oth.sides[i].pWinding)
			sides[i].pWinding = oth.sides[i].pWinding->Copy();
	}

}

bool bspBrush::createBrushWindings(const XPlaneSet& planes)
{
	int			i, j;
	XWinding	*w;
	const Planef*		pPlane;
	BspSide*	pSide;

	for (i = 0; i < numsides; i++)
	{
		pSide = &sides[i];
		pPlane = &planes[pSide->planenum];
		w = new XWinding(*pPlane);

		for (j = 0; j < numsides && w; j++)
		{
			if (i == j) {
				continue;
			}
			if (sides[j].planenum == (pSide->planenum ^ 1)) {
				continue;		// back side clipaway
			}

			w = w->Clip(planes[sides[j].planenum ^ 1], 0.01f);
		}
		if (pSide->pWinding) {
			delete pSide->pWinding;
		}
		pSide->pWinding = w;
	}

	return boundBrush(planes);
}

bool bspBrush::boundBrush(const XPlaneSet& planes)
{
	int			i, j;
	XWinding	*w;

	bounds.clear();
	for (i = 0; i < numsides; i++) {
		w = sides[i].pWinding;
		if (!w)
			continue;
		for (j = 0; j < w->GetNumPoints(); j++)
			bounds.add((*w)[j].asVec3());
	}

	for (i = 0; i < 3; i++) {
		if (bounds.min[i] < bsp::MIN_WORLD_COORD || bounds.max[i] > bsp::MAX_WORLD_COORD
			|| bounds.min[i] >= bounds.max[i])
		{
			// calculate a pos.
			Planef::Description Dsc;
			const Planef* plane = nullptr;
			if (numsides > 0)
				plane = &planes[sides[0].planenum];

			X_WARNING("Brush", "Entity %i, Brush %i, Sides %i: failed to calculate brush bounds (%s)",
				entityNum, brushNum, numsides, plane ? plane->toString(Dsc) : "");
			return false;
		}
	}

	return true;
}


float bspBrush::Volume(const XPlaneSet& planes)
{
	int			i;
	XWinding	*w;
	Vec3f		corner;
	float		d, area, volume;
	const Planef		*plane;


	// grab the first valid point as the corner

	w = nullptr;
	for (i = 0; i < numsides; i++) {
		w = sides[i].pWinding;
		if (w)
			break;
	}
	if (!w) {
		return 0;
	}

	corner = (*w)[0].asVec3();

	// make tetrahedrons to all other faces
	volume = 0;
	for (; i < numsides; i++)
	{
		w = sides[i].pWinding;
		if (!w)
			continue;
		plane = &planes[sides[i].planenum];
		d = -plane->distance(corner);
		area = w->GetArea();
		volume += d * area;
	}

	volume /= 3;
	return volume;
}


BrushPlaneSide::Enum bspBrush::BrushMostlyOnSide(const Planef& plane)
{
	int			i, j;
	XWinding	*w;
	float		d, max;
	BrushPlaneSide::Enum side;

	max = 0;
	side = BrushPlaneSide::FRONT;
	for (i = 0; i < numsides; i++) 
	{
		w = sides[i].pWinding;

		if (!w)
			continue;

		for (j = 0; j < w->GetNumPoints(); j++)
		{
			d = plane.distance((*w)[j].asVec3());
			if (d > max)
			{
				max = d;
				side = BrushPlaneSide::FRONT;
			}
			if (-d > max)
			{
				max = -d;
				side = BrushPlaneSide::BACK;
			}
		}
	}
	return side;
}

// ------------------------------ Entity -----------------------------------

LvlEntity::LvlEntity()
{
	pBrushes = nullptr;
	pPatches = nullptr;
	mapEntity = nullptr;

	numBrushes = 0;
	numPatches = 0;
}

// ------------------------------ Tris -----------------------------------

bspTris::bspTris()
{
	next = nullptr;
}

// ------------------------------ Face -----------------------------------

bspFace::bspFace()
{
	core::zero_this(this);
}

// ------------------------------ Node -----------------------------------

bspNode::bspNode()
{
	core::zero_this(this);
}

// ------------------------------ Tree -----------------------------------

bspTree::bspTree()
{
	core::zero_this(this);
}