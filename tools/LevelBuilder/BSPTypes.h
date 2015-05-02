#pragma once


#ifndef X_BSP_TYPES_H_
#define X_BSP_TYPES_H_

#include <IModel.h>

#include <Containers\LinkedListIntrusive.h>

#include <String\GrowingStringTable.h>

#include "BSPData.h"

X_NAMESPACE_DECLARE(mapfile,
class XMapFile;
class XMapEntity;
class XMapBrush;
class XMapPatch;
);


// ----------------------------------------
#define PLANENUM_LEAF           -1


X_DECLARE_ENUM(ShadowOpt)(NONE, MERGE_SURFACES, CULL_OCCLUDED, CLIP_OCCLUDERS, CLIP_SILS, SIL_OPTIMIZE);


struct Settings
{
	Settings() {
		noPatches = false;
		noTJunc = false;
		nomerge = false;
		noFlood = false;
		noOptimize = true;

		noClipSides = false;
		noLightCarve = false;

		shadowOptLevel = ShadowOpt::NONE;
	}

	bool noPatches;
	bool noTJunc;
	bool nomerge;
	bool noFlood;
	bool noOptimize;

	bool noClipSides;		// don't cut sides by solid leafs, use the entire thing
	bool noLightCarve;		// extra triangle subdivision by light frustums

	ShadowOpt::Enum	shadowOptLevel;
};

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

// --------------------------------------------------------

extern Settings gSettings;

// typedef Vec4f textureVectors[2];


struct LvlEntity
{
	LvlEntity();

	Vec3f				origin;

	struct bspBrush*	pBrushes;
	struct bspTris*		pPatches;

	size_t	numBrushes;
	size_t  numPatches;

	mapfile::XMapEntity*	mapEntity;		// points to the map data this was made from.
};


struct LvlMaterial
{
	core::StackString<level::MAP_MAX_MATERIAL_LEN> name;
	Vec2f				  matRepeate;
	Vec2f				  shift;
	float				  rotate;
};

// a brush side, typically 6.
struct BspSide
{
	BspSide() : planenum(0), pWinding(nullptr), pVisibleHull(nullptr) {}

	int				planenum;
	bool			visible;
	bool			culled;

	LvlMaterial		material;

	XWinding*		pWinding;		// only clipped to the other sides of the brush
	XWinding*       pVisibleHull;   // convex hull of all visible fragments 
};


struct bspBrush
{
	bspBrush();
	bspBrush(const bspBrush& oth);


	bool createBrushWindings(const XPlaneSet& planes);
	bool boundBrush(const XPlaneSet& planes);
	float Volume(const XPlaneSet& planes);
	
	BrushPlaneSide::Enum BrushMostlyOnSide(const Planef& plane);


public:
	struct bspBrush*	next;

	// used for poviding helpful error msg's
	int					entityNum;
	int					brushNum;

	AABB				bounds;
	bool				opaque;
	bool				detail;
	bool				allsidesSameMat; // all the sides use same material.
	bool				__pad;

	int					numsides;
	// 6 are members. but if num sides is > 6 then memory directly after
	// is valid BspSide structures.
	BspSide				sides[6]; 
};

struct bspTris
{
	bspTris();

	bspTris*			next;
	xVert				v[3];
};

struct bspFace
{
	bspFace();

	struct bspFace*		next;

	int					planenum;
	bool				portal;			
	bool				checked;		
	XWinding*			w;
};


struct bspNode
{
	bspNode();

	struct bspNode*		parent;

	int					planenum;                   // -1 = leaf node 
	AABB				bounds;


	// nodes only 
	BspSide*			side;          // the side that created the node 
	struct bspNode*     children[2];
	int					tinyportals;


	// leafs only 
	bool opaque;                    // view can never be inside 
	bool areaportal;
	bool _pad[2];

	int cluster;                   // for portalfile writing 
	int area;                       // for areaportals 


	bspBrush             *brushlist;     // fragments of all brushes in this leaf 
//	drawSurfRef_t       *drawSurfReferences;

	int occupied;                       // 1 or greater can reach entity 
};


struct bspTree
{
	bspTree();

	bspNode*	headnode;
	bspNode		outside_node;
	AABB		bounds;
};



X_DECLARE_ENUM(DrawSurfaceType)(FACE,DECAL,PATCH);


struct bspDrawSurface
{
	DrawSurfaceType::Enum type;


	bspBrush             *mapBrush;
//	parseMesh_t         *mapMesh;
//	sideRef_t           *sideRef;


	// verts
	int numVerts;                           
	level::Vertex* pVerts;
	// indexes
	int numIndexes;
	int* pIndexes;


	// texture coordinate range monitoring for hardware with limited
	// texcoord precision (in texel space) 
	float bias[ 2 ];
	int texMins[ 2 ], texMaxs[ 2 ], texRange[ 2 ];

	AABB bounds;

	int entityNum;
	int surfaceNum;
	int outputNum;
	int planeNum;

	bool planar;
};



typedef core::GrowingStringTableUnique<256, 16, 4, uint32_t> StringTableType;

struct AreaModel
{
	AreaModel();

	bool BelowLimits(void);
	void BeginModel(void);
	void EndModel(void);

	core::Array<model::SubMeshHeader> meshes;
	core::Array<level::Vertex> verts;
	core::Array<model::Face> indexes;

	model::MeshHeader model;
};

// used to build up submeshes.
// so faces with same materials are grouped into meshes.
struct AreaSubMesh
{
	AreaSubMesh() : verts_(g_arena), indexes_(g_arena) {}

	void AddVert(const level::Vertex& vert) {
		verts_.append(vert);
	}

	uint32_t matNameID_;

	// index's for this sub mesh.
	// merged into AreaModel list at end.
	core::Array<level::Vertex> verts_;
	core::Array<model::Face> indexes_;
};


class LvlArea
{
	typedef core::HashMap<core::string, AreaSubMesh> AreaMeshMap;
	typedef core::Array<LvlEntity> AreaEntsArr;
	typedef core::Array<LvlArea> ConnectAreasArr;
	typedef core::Array<AABB> CullSectionsArr;
public:
	LvlArea();

	void AreaBegin(void);
	void AreaEnd(void);
	AreaSubMesh* MeshForSide(const BspSide& side, StringTableType& stringTable);

public:
	// area has one model.
	AreaModel model;

	AreaMeshMap areaMeshes;
	AreaEntsArr	entities;
	ConnectAreasArr connectedAreas;
	// we split the area up into a optimal avg'd collection of AABB's
	// which are turned into worker jobs.
	CullSectionsArr cullSections;

	// copy of the model values.
	AABB boundingBox;
	Sphere boundingSphere;
};

class LvlBuilder
{
public:
	LvlBuilder();

	bool LoadFromMap(mapfile::XMapFile* map);
	bool ProcessModels(void);

	bool save(const char* filename);

private:

	bspBrush* AllocBrush(int numSides);
	bspBrush* CopyBrush(bspBrush* pOth);
	void FreeBrush(bspBrush* pBrush);

	bspFace* AllocBspFace(void);
	void FreeBspFace(bspFace* pFace);

	bspTree* AllocTree(void);
	void FreeTree(bspTree* pTree);

	bspNode* AllocNode(void);
	void FreeNode(bspNode* pNode);

	bspDrawSurface* AllocDrawSurface(DrawSurfaceType::Enum type);

	// -----------

	int FindFloatPlane(const Planef& plane);
	
	bool processMapEntity(LvlEntity& ent, mapfile::XMapEntity* mapEnt);
	bool processBrush(LvlEntity& ent, mapfile::XMapBrush* brush, int ent_idx);
	bool processPatch(LvlEntity& ent, mapfile::XMapPatch* brush, int ent_idx);

	bool removeDuplicateBrushPlanes(bspBrush* pBrush);

private:

	bool ProcessModel(const LvlEntity& ent);
	bool ProcessWorldModel(const LvlEntity& ent);

private:
	core::Array<LvlEntity>		entities_;
	core::Array<LvlArea>		areas_;

	core::GrowingStringTableUnique<256, 16, 4, uint32_t> stringTable_;


	BSPData		data_;
	XPlaneSet	planes;
	AABB		mapBounds;
	Vec3f		blockSize_;

	struct Stats
	{
		int		numEntities;
		int		numPatches;
		int		numBrushes;
		int		numAreaPortals;
		int		numFaceLeafs;
	};

	Stats stats_;
};


#endif // X_BSP_TYPES_H_