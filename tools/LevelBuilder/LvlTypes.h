#pragma once


#ifndef X_LVL_TYPES_H_
#define X_LVL_TYPES_H_

#include <IModel.h>
#include <IMaterial.h>
#include <String\GrowingStringTable.h>

#include "BSPTypes.h"

#include <set>


X_NAMESPACE_DECLARE(mapfile,
class XMapFile;
class XMapEntity;
class XMapBrush;
class XMapPatch;
);

struct BrushPlaneSide
{
	enum Enum
	{
		FRONT = 1,
		BACK,
		BOTH = FRONT | BACK, // same but yer ^^
		FACING
	};
};

typedef core::GrowingStringTableUnique<256, 16, 4, uint32_t> StringTableType;

struct LvlMaterial
{
	LvlMaterial();

	const engine::MaterialFlags getFlags(void) const;

public:
	core::StackString<level::MAP_MAX_MATERIAL_LEN> name;
	Vec2f				  matRepeate;
	Vec2f				  shift;
	float				  rotate;
	engine::IMaterial*	  pMaterial;
};

struct LvlBrushSide
{
	LvlBrushSide();
	LvlBrushSide(const LvlBrushSide& oth);

	LvlBrushSide& operator=(const LvlBrushSide& oth);

	int32_t	planenum;

	bool visible;
	bool culled;
	bool __pad[2];

	LvlMaterial matInfo;

	XWinding*		pWinding;		// only clipped to the other sides of the brush
	XWinding*       pVisibleHull;   // convex hull of all visible fragments 
};

struct LvlBrush
{
	LvlBrush();
	LvlBrush(const LvlBrush& oth);

	LvlBrush& operator=(const LvlBrush& oth);

	bool createBrushWindings(const XPlaneSet& planes);
	bool boundBrush(const XPlaneSet& planes);
	bool calculateContents(void);
	float Volume(const XPlaneSet& planes);

	BrushPlaneSide::Enum BrushMostlyOnSide(const Planef& plane) const;

	size_t FilterBrushIntoTree_r(XPlaneSet& planes, bspNode* node);

	void Split(XPlaneSet& planes, int32_t planenum, LvlBrush*& pFront, LvlBrush*& pBack);

public:
	typedef core::Array<LvlBrushSide> SidesArr;

	struct LvlBrush* pOriginal;

	AABB bounds;

	int32_t	entityNum;
	int32_t	brushNum;

	bool opaque;
	bool allsidesSameMat; // all the sides use same material.
	bool __pad[2];

	// the combined flags of all sides.
	// so if one side has portal type.
	// this will contain portal flag.
	engine::MaterialFlags combinedMatFlags;

	SidesArr sides;
};



struct LvlTris
{
	LvlTris();

	engine::IMaterial*	  pMaterial;

	xVert verts[3];
};

struct LvlInterPortal
{
	LvlInterPortal();

	int32_t area0;
	int32_t area1;
	const LvlBrushSide* pSide;
};

struct LvlEntity
{
	typedef core::Array<LvlBrush> LvlBrushArr;
	typedef core::Array<LvlTris> TrisArr;
	typedef core::Array<LvlInterPortal> LvlInterPortalArr;
public:
	LvlEntity();
	~LvlEntity();

	bool FindInterAreaPortals(void);
	bool FindInterAreaPortals_r(bspNode* node);


	bool MakeStructuralFaceList(void);
	bool FacesToBSP(XPlaneSet& planeSet);
	bool MakeTreePortals(XPlaneSet& planeSet);
	bool FilterBrushesIntoTree(XPlaneSet& planeSet);
	bool FloodEntities(XPlaneSet& planeSet, core::Array<LvlEntity>& ents, mapfile::XMapFile* pMap);
	bool FillOutside(void);
	bool ClipSidesByTree(XPlaneSet& planeSet);
	bool FloodAreas(void);
	bool PruneNodes(void);


private:

	bool PlaceOccupant(XPlaneSet& planeSet, bspNode* node, size_t& floodedNum);

	void ClipSideByTree_r(XWinding* w, LvlBrushSide& side, bspNode *node);
	void FindAreas_r(bspNode *node, size_t& numAreas);

	static bool CheckAreas_r(bspNode* pNode);

public:
	Vec3f origin;
	Vec3f angle; // euelr
	AABB bounds; // set for models, only currently.

	LvlBrushArr brushes;
	TrisArr patches;
	LvlInterPortalArr interPortals;
	// bsp data.
	bspFace* bspFaces;
	bspTree bspTree;

	size_t numAreas;

	level::ClassType::Enum classType;

	mapfile::XMapEntity*	mapEntity;		// points to the map data this was made from.
};


struct AreaModel
{
	AreaModel();

	bool BelowLimits(void);
	void BeginModel(void);
	void EndModel(void);

	core::Array<model::SubMeshHeader> meshes;
	core::Array<level::Vertex> verts;
	core::Array<model::Face> faces;

	model::MeshHeader model;
};

// used to build up submeshes.
// so faces with same materials are grouped into meshes.
struct AreaSubMesh
{
	AreaSubMesh() : verts_(g_arena), faces_(g_arena) {}

	void AddVert(const level::Vertex& vert) {
		verts_.append(vert);
	}

	core::StackString<level::MAP_MAX_MATERIAL_LEN> matName_;
	uint32_t matNameID_;

	// index's for this sub mesh.
	// merged into AreaModel list at end.
	core::Array<level::Vertex> verts_;
	core::Array<model::Face> faces_;
};


class LvlArea
{
	typedef core::HashMap<core::string, AreaSubMesh> AreaMeshMap;
	typedef core::Array<uint32_t> AreaEntsRef;
//	typedef core::Array<LvlEntity*> AreaEntsArr;
//	typedef core::Array<AABB> CullSectionsArr;
public:
	LvlArea();

	void AreaBegin(void);
	void AreaEnd(void);
	AreaSubMesh* MeshForSide(const LvlBrushSide& side, StringTableType& stringTable);

public:
	// area has one model.
	AreaModel model;

	AreaMeshMap areaMeshes;
	AreaEntsRef entRefs;
//	AreaEntsArr	entities;

	// we split the area up into a optimal avg'd collection of AABB's
	// which are turned into worker jobs.
	//CullSectionsArr cullSections;

	// copy of the model values.
	AABB boundingBox;
	Sphere boundingSphere;
};


typedef core::Array<LvlEntity> LvlEntsArr;
typedef core::Array<LvlArea> LvlAreaArr;

struct LvlStats
{
	LvlStats()  {
		core::zero_this(this);
	}
	int32_t	numEntities;
	int32_t	numPatches;
	int32_t	numBrushes;
	int32_t	numAreaPortals;
	int32_t	numFaceLeafs;
};



#endif // !X_LVL_TYPES_H_