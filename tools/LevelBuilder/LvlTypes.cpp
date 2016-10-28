#include "stdafx.h"
#include "LvlTypes.h"


LvlMaterial::LvlMaterial()
{
	rotate = 0.f;
	pMaterial = nullptr;
}


const engine::MaterialFlags LvlMaterial::getFlags(void) const
{
	X_ASSERT_NOT_NULL(pMaterial);
	return pMaterial->getFlags();
}

// ==========================================

LvlBrushSide::LvlBrushSide() : 
planenum(0), 
visible(true),
culled(false),
pWinding(nullptr), 
pVisibleHull(nullptr)
{
	core::zero_object(__pad);
}

LvlBrushSide::LvlBrushSide(const LvlBrushSide& oth) : matInfo(oth.matInfo)
{
	planenum = oth.planenum;

	visible = oth.visible;
	culled = oth.culled;

//	matInfo = oth.matInfo;

	// null them first.
	pWinding = nullptr;
	pVisibleHull = nullptr;

	if (oth.pWinding){
		pWinding = oth.pWinding->Copy(g_arena);
	}
	if (oth.pVisibleHull){
		pVisibleHull = oth.pVisibleHull->Copy(g_arena);
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
		pWinding = oth.pWinding->Copy(g_arena);
	}
	if (oth.pVisibleHull){
		pVisibleHull = oth.pVisibleHull->Copy(g_arena);
	}
	return *this;
}

// ==========================================

LvlTris::LvlTris()
{
	pMaterial = nullptr;
}

// ==========================================

LvlInterPortal::LvlInterPortal()
{
	area0 = -1;
	area1 = -1;
	pSide = nullptr;
}

// ==========================================

LvlEntity::LvlEntity() :
brushes(g_arena),
patches(g_arena),
interPortals(g_arena),
numAreas(0)
{
	classType = level::ClassType::UNKNOWN;
	bspFaces = nullptr;
	mapEntity = nullptr;
}

LvlEntity::~LvlEntity()
{
	if (bspFaces)
	{
		// it's a linked list of nodes.
		bspFace* pFace = bspFaces;
		bspFace* pNext = nullptr;
		for (; pFace; pFace = pNext)
		{
			pNext = pFace->pNext;
			X_DELETE(pFace, g_arena);
		}
	}
}
// ==========================================


LvlBrush::LvlBrush() :
pOriginal(nullptr),
sides(g_arena)
{
	entityNum = -1;
	brushNum = -1;

	bounds.clear();

	opaque = false;
	allsidesSameMat = true;
}

LvlBrush::LvlBrush(const LvlBrush& oth) :
sides(g_arena)
{
	pOriginal = oth.pOriginal;

	// used for poviding helpful error msg's
	entityNum = oth.entityNum;
	brushNum = oth.brushNum;

	bounds = oth.bounds;
	opaque = oth.opaque;
	allsidesSameMat = oth.allsidesSameMat;
	//	detail = oth.detail;

	combinedMatFlags = oth.combinedMatFlags;

	sides.resize(oth.sides.size());

	// cop sides
	for (size_t i = 0; i < oth.sides.size(); i++)
	{
		sides[i] = oth.sides[i];
	}
}

LvlBrush& LvlBrush::operator = (const LvlBrush& oth)
{
	pOriginal = oth.pOriginal;

	// used for poviding helpful error msg's
	entityNum = oth.entityNum;
	brushNum = oth.brushNum;

	bounds = oth.bounds;
	opaque = oth.opaque;
	allsidesSameMat = oth.allsidesSameMat;
	//	detail = oth.detail;

	combinedMatFlags = oth.combinedMatFlags;

	sides.clear();
	sides.resize(oth.sides.size());

	// cop sides
	for (size_t i = 0; i < oth.sides.size(); i++)
	{
		sides[i] = oth.sides[i];
	}

	return *this;
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

			if(!w->clip(planes[sides[j].planenum ^ 1], 0.01f)) {
				X_DELETE_AND_NULL(w, g_arena);
			}
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
	size_t j;
	XWinding* w;

	bounds.clear();
	for (i = 0; i < sides.size(); i++)
	{
		w = sides[i].pWinding;
		if (!w) {
			continue;
		}
		for (j = 0; j < w->getNumPoints(); j++) {
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

bool LvlBrush::calculateContents(void)
{
	core::StackString<level::MAP_MAX_MATERIAL_LEN> MatName;
	SidesArr::ConstIterator it;

	if (sides.isEmpty()) {
		return false;
	}

	MatName = sides[0].matInfo.name;

	// a brush is only opaque if all sides are opaque
	opaque = true;
	allsidesSameMat = true;

	combinedMatFlags.Clear();

	for (it = sides.begin(); it != sides.end(); ++it)
	{
		const LvlBrushSide& side = *it;

		if (!side.matInfo.pMaterial) {
			X_ERROR("Brush", "material not found for brush side. ent: %i brush: %i",
				entityNum, brushNum);
			return false;
		}

		engine::Material* pMat = side.matInfo.pMaterial;

		combinedMatFlags |= pMat->getFlags();

		if (pMat->getFlags().IsSet(engine::MaterialFlag::PORTAL)) {
			opaque = false;
		}
		else if (pMat->getCoverage() != engine::MaterialCoverage::OPAQUE) {
			opaque = false;
		}

		if (MatName != side.matInfo.name) {
			allsidesSameMat = false;
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
		area = w->getArea();
		volume += d * area;
	}

	volume /= 3;
	return volume;
}


BrushPlaneSide::Enum LvlBrush::BrushMostlyOnSide(const Planef& plane) const
{
	size_t i;
	size_t j;
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

		for (j = 0; j < w->getNumPoints(); j++)
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
