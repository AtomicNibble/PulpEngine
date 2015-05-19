#include "stdafx.h"
#include "LvlTypes.h"


LvlMaterial::LvlMaterial()
{
	rotate = 0.f;
	pMaterial = nullptr;
}

// ==========================================

LvlBrushSide::LvlBrushSide() : 
planenum(0), 
visible(true),
culled(false),
pWinding(nullptr), 
pVisibleHull(nullptr)
{

}

LvlBrushSide::LvlBrushSide(const LvlBrushSide& oth)
{
	planenum = oth.planenum;

	visible = oth.visible;
	culled = oth.culled;

	matInfo = oth.matInfo;

	// null them first.
	pWinding = nullptr;
	pVisibleHull = nullptr;

	if (oth.pWinding){
		pWinding = oth.pWinding->Copy();
	}
	if (oth.pVisibleHull){
		pVisibleHull = oth.pVisibleHull->Copy();
	}
}

LvlBrushSide& LvlBrushSide::operator = (const LvlBrushSide& oth)
{
	// nned to delete them if set.
	if (pWinding) {
		X_ASSERT_NOT_IMPLEMENTED();
	}
	if (pVisibleHull) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	planenum = oth.planenum;

	visible = oth.visible;
	culled = oth.culled;

	matInfo = oth.matInfo;

	// null them first.
	pWinding = nullptr;
	pVisibleHull = nullptr;

	if (oth.pWinding){
		pWinding = oth.pWinding->Copy();
	}
	if (oth.pVisibleHull){
		pVisibleHull = oth.pVisibleHull->Copy();
	}
	return *this;
}

// ==========================================

LvlTris::LvlTris()
{
	pMaterial = nullptr;
}

// ==========================================

LvlEntity::LvlEntity() :
brushes(g_arena),
patches(g_arena)
{
	bspFaces = nullptr;
	mapEntity = nullptr;
}

LvlEntity::~LvlEntity()
{
	if (bspFaces) {
		X_ASSERT_NOT_IMPLEMENTED();
//		X_DELETE(bspFaces, g_arena);
	}
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
	for (size_t i = 0; i < oth.sides.size(); i++)
	{
		sides[i] = oth.sides[i];
	}
}

bool LvlBrush::createBrushWindings(const XPlaneSet& planes)
{
	size_t i, j;
	XWinding* w;
	const Planef* pPlane;
	LvlBrushSide* pSide;

	for (i = 0; i < sides.size(); i++)
	{
		pSide = &sides[i];
		pPlane = &planes[pSide->planenum];
		w = X_NEW(XWinding,g_arena, "BrushWinding")(*pPlane);

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
			X_DELETE(pSide->pWinding, g_arena);
		}
		pSide->pWinding = w;
	}

	return boundBrush(planes);
}

bool LvlBrush::boundBrush(const XPlaneSet& planes)
{
	size_t i;
	int32_t j;
	XWinding* w;

	bounds.clear();
	for (i = 0; i < sides.size(); i++)
	{
		w = sides[i].pWinding;
		if (!w) {
			continue;
		}
		for (j = 0; j < w->GetNumPoints(); j++) {
			bounds.add((*w)[j].asVec3());
		}
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

			X_WARNING("LvlBrush", "Entity %i, Brush %i, Sides %i: failed "
				"to calculate brush bounds (%s)",
				entityNum, brushNum, sides.size(), plane ? plane->toString(Dsc) : "");
			return false;
		}
	}

	return true;
}


float LvlBrush::Volume(const XPlaneSet& planes)
{
	size_t i;
	XWinding* w;
	Vec3f corner;
	float d, area, volume;
	const Planef* plane;

	// grab the first valid point as the corner
	w = nullptr;
	for (i = 0; i < sides.size(); i++) {
		w = sides[i].pWinding;
		if (w) {
			break;
		}
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
		if (!w) {
			continue;
		}
		plane = &planes[sides[i].planenum];
		d = -plane->distance(corner);
		area = w->GetArea();
		volume += d * area;
	}

	volume /= 3;
	return volume;
}


BrushPlaneSide::Enum LvlBrush::BrushMostlyOnSide(const Planef& plane) const
{
	size_t i;
	int32_t j;
	XWinding* w;
	float d, max;
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
