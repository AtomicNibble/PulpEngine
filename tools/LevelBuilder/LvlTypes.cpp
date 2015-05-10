#include "stdafx.h"
#include "LvlTypes.h"


LvlBrushSide::LvlBrushSide() : 
planenum(0), 
visible(true),
culled(false),
pWinding(nullptr), 
pVisibleHull(nullptr)
{

}


LvlEntity::LvlEntity() :
brushes(g_arena)
{
	mapEntity = nullptr;
}

// ==========================================


LvlBrush::LvlBrush() :
sides(g_arena)
{
	entityNum = -1;
	brushNum = -1;

	bounds.clear();

	opaque = true;
	allsidesSameMat = true;
}

LvlBrush::LvlBrush(const LvlBrush& oth) :
sides(g_arena)
{
	// used for poviding helpful error msg's
	entityNum = oth.entityNum;
	brushNum = oth.brushNum;

	bounds = oth.bounds;
	opaque = oth.opaque;
	//	detail = oth.detail;

	sides.resize(oth.sides.size());

	// cop sides
	for (int i = 0; i < oth.sides.size(); i++)
	{
		sides[i] = oth.sides[i];
	}
}

bool LvlBrush::createBrushWindings(const XPlaneSet& planes)
{
	size_t	i, j;
	XWinding	*w;
	const Planef*		pPlane;
	LvlBrushSide*	pSide;

	for (i = 0; i < sides.size(); i++)
	{
		pSide = &sides[i];
		pPlane = &planes[pSide->planenum];
		w = new XWinding(*pPlane);

		for (j = 0; j < sides.size() && w; j++)
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

bool LvlBrush::boundBrush(const XPlaneSet& planes)
{
	size_t		i, j;
	XWinding	*w;

	bounds.clear();
	for (i = 0; i < sides.size(); i++) {
		w = sides[i].pWinding;
		if (!w)
			continue;
		for (j = 0; j < w->GetNumPoints(); j++)
			bounds.add((*w)[j].asVec3());
	}

	for (i = 0; i < 3; i++) 
	{
		if (bounds.min[i] < level::MIN_WORLD_COORD || bounds.max[i] > level::MAX_WORLD_COORD
			|| bounds.min[i] >= bounds.max[i])
		{
			// calculate a pos.
			Planef::Description Dsc;
			const Planef* plane = nullptr;
			if (sides.size() > 0) {
				plane = &planes[sides[0].planenum];
			}

			X_WARNING("LvlBrush", "Entity %i, Brush %i, Sides %i: failed to calculate brush bounds (%s)",
				entityNum, brushNum, numsides, plane ? plane->toString(Dsc) : "");
			return false;
		}
	}

	return true;
}


float LvlBrush::Volume(const XPlaneSet& planes)
{
	int			i;
	XWinding*	w;
	Vec3f		corner;
	float		d, area, volume;
	const Planef* plane;

	// grab the first valid point as the corner
	w = nullptr;
	for (i = 0; i < sides.size(); i++) {
		w = sides[i].pWinding;
		if (w)
			break;
	}
	if (!w) {
		return 0.f;
	}

	corner = (*w)[0].asVec3();

	// make tetrahedrons to all other faces
	volume = 0;
	for (; i < sides.size(); i++)
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


BrushPlaneSide::Enum LvlBrush::BrushMostlyOnSide(const Planef& plane)
{
	int			i, j;
	XWinding*	w;
	float		d, max;
	BrushPlaneSide::Enum side;

	max = 0;
	side = BrushPlaneSide::FRONT;
	for (i = 0; i < sides.size(); i++)
	{
		w = sides[i].pWinding;

		if (!w) {
			continue;
		}

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
