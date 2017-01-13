#pragma once


#ifndef X_LVL_TYPES_H_
#define X_LVL_TYPES_H_

#include <IPhysics.h>
#include <IModel.h>
#include <ILevel.h>

#include <String\GrowingStringTable.h>

#include "BSPTypes.h"



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
	engine::Material*	  pMaterial;
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

	engine::Material*	  pMaterial;

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

	bool AddMapTriToAreas(XPlaneSet& planeSet, const LvlTris& tris);
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


// contains all the collision data for a area.
// we will support tri mesh and hieght fields.
struct AreaCollsiion
{
	// a single chunck of collision data.
	struct TriMeshData
	{
		TriMeshData(core::MemoryArenaBase* arena);

		bool cook(physics::IPhysicsCooking* pCooking);

		X_INLINE void AddVert(const level::Vertex& vert) {
			verts.append(vert);
		}
		X_INLINE void AddFace(const model::Face& face) {
			faces.append(face);
		}

		core::Array<level::Vertex> verts;
		core::Array<model::Face> faces;
		core::Array<uint8_t> cookedData;
	};

	// a collection of collision data for a given set of group flags.
	struct GroupBucket
	{
		typedef core::Array<TriMeshData> TriMesgDataArr;

		GroupBucket(physics::GroupFlags groupFlags, core::MemoryArenaBase* arena);

		physics::GroupFlags getGroupFlags(void) const;
		const TriMesgDataArr& getTriMeshDataArr(void) const;
		
		TriMeshData& getCurrentTriMeshData(void);
		void beginNewTriMesh(void); // move to a new block of data, used to break collision data up into smaller chuncks for potential performance gains.

	private:
		physics::GroupFlags groupFlags_;
		TriMesgDataArr triMeshData_;
	};

	typedef core::Array<GroupBucket> ColGroupBucketArr;


public:
	AreaCollsiion(core::MemoryArenaBase* arena);

	size_t numGroups(void) const;
	const ColGroupBucketArr& getGroups(void) const;

	GroupBucket& getBucket(physics::GroupFlags flags);

private:
	ColGroupBucketArr colGroupBuckets_;
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
	AreaSubMesh();

	X_INLINE void AddVert(const level::Vertex& vert) {
		verts_.append(vert);
	}
	X_INLINE void AddFace(const model::Face& face) {
		faces_.append(face);
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
	typedef core::Array<uint32_t> AreaRefs;

public:
	LvlArea();

	void AreaBegin(void);
	void AreaEnd(void);
	AreaSubMesh* MeshForSide(const LvlBrushSide& side, StringTableType& stringTable);
	AreaSubMesh* MeshForMat(const core::string& matName, StringTableType& stringTable);

public:
	// area has one model.
	AreaModel model;
	AreaCollsiion collision;

	AreaMeshMap areaMeshes;
	AreaRefs entRefs;
	AreaRefs modelsRefs;

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