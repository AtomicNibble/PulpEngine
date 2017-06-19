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

LvlBrushSide::~LvlBrushSide()
{
	if (pWinding) {
		X_DELETE(pWinding, g_arena);
	}
	if (pVisibleHull) {
		X_DELETE(pVisibleHull, g_arena);
	}
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
	pBspFaces = nullptr;
	pMapEntity = nullptr;
}

LvlEntity::~LvlEntity()
{
	if (pBspFaces)
	{
		// it's a linked list of nodes.
		bspFace* pFace = pBspFaces;
		bspFace* pNext = nullptr;
		for (; pFace; pFace = pNext)
		{
			pNext = pFace->pNext;
			X_DELETE(pFace, g_arena);
		}
	}

	if (bspTree_.headnode) {
		bspTree_.headnode->FreeTree_r();
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
	const Planef* pPlane;
	LvlBrushSide* pSide;

	for (size_t i = 0; i < sides.size(); i++)
	{
		pSide = &sides[i];
		pPlane = &planes[pSide->planenum];
		auto* pWinding = X_NEW(XWinding,g_arena, "BrushWinding")(*pPlane);

		for (size_t j = 0; j < sides.size() && pWinding; j++)
		{
			if (i == j) {
				continue;
			}
			if (sides[j].planenum == (pSide->planenum ^ 1)) {
				continue;		// back side clipaway
			}

			if(!pWinding->clip(planes[sides[j].planenum ^ 1], 0.01f)) {
				X_DELETE_AND_NULL(pWinding, g_arena);
			}
		}
		if (pSide->pWinding) {
			X_DELETE(pSide->pWinding, g_arena);
		}
		pSide->pWinding = pWinding;
	}

	return boundBrush(planes);
}

bool LvlBrush::boundBrush(const XPlaneSet& planes)
{
	bounds.clear();
	for (size_t i = 0; i < sides.size(); i++)
	{
		auto* pWinding = sides[i].pWinding;
		if (!pWinding) {
			continue;
		}
		for (size_t j = 0; j < pWinding->getNumPoints(); j++) {
			bounds.add((*pWinding)[j].asVec3());
		}
	}

	for (size_t i = 0; i < 3; i++)
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
	if (sides.isEmpty()) {
		return false;
	}

	MaterialName matName = sides[0].matInfo.name;

	// a brush is only opaque if all sides are opaque
	opaque = true;
	allsidesSameMat = true;

	combinedMatFlags.Clear();

	for (const auto& side : sides)
	{
		if (!side.matInfo.pMaterial) {
			X_ERROR("Brush", "material not found for brush side. ent: %i brush: %i",
				entityNum, brushNum);
			return false;
		}

		engine::Material* pMat = side.matInfo.pMaterial;
		const auto flags = pMat->getFlags();

		combinedMatFlags |= flags;

		if (flags.IsSet(engine::MaterialFlag::PORTAL)) {
			opaque = false;
		}
		else if (pMat->getCoverage() != engine::MaterialCoverage::OPAQUE) {
			opaque = false;
		}

		if (matName != side.matInfo.name) {
			allsidesSameMat = false;
		}
	}

	return true;
}


float LvlBrush::Volume(const XPlaneSet& planes)
{
	// grab the first valid point as the corner
	size_t i;
	XWinding* w = nullptr;
	for (i = 0; i < sides.size(); i++) {
		w = sides[i].pWinding;
		if (w) {
			break;
		}
	}
	if (!w) {
		return 0.f;
	}

	Vec3f corner = (*w)[0].asVec3();
	float d, area, volume;
	const Planef* plane;

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
	float max = 0;
	BrushPlaneSide::Enum side = BrushPlaneSide::FRONT;
	for (size_t i = 0; i < sides.size(); i++)
	{
		auto* w = sides[i].pWinding;

		if (!w) {
			continue;
		}

		for (size_t j = 0; j < w->getNumPoints(); j++)
		{
			float d = plane.distance((*w)[j].asVec3());
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
