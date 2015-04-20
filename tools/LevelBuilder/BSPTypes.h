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
	core::StackString<bsp::MAP_MAX_MATERIAL_LEN> name;
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
	bsp::Vertex* pVerts;
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


#if 0

// chains of mapTri_t are the general unit of processing
typedef struct mapTri_s 
{
	struct mapTri_s*	next;

	void*				mergeGroup;		// we want to avoid merging triangles
	int					planeNum;		// not set universally, just in some areas

	xVert				v[3];
	const struct hashVert_s* hashVert[3];
	struct optVertex_s* optVert[3];
} mapTri_t;


typedef struct 
{
	int					width, height;
	xVert*				verts;
} mesh_t;

typedef struct parseMesh_s 
{
	struct parseMesh_s* next;
	mesh_t				mesh;
} parseMesh_t;

typedef struct bspface_s 
{
	struct bspface_s*	next;
	int					planenum;
	bool				portal;			// all portals will be selected before
	// any non-portals
	bool				checked;		// used by SelectSplitPlaneNum()
	XWinding*			w;
} bspface_t;

typedef struct 
{
	Vec4f		v[2];					// the offset value will always be in the 0.0 to 1.0 range
} textureVectors_t;


typedef struct side_s 
{
	int					planenum;

	textureVectors_t	texVec;

	XWinding*			winding;		// only clipped to the other sides of the brush
	XWinding*			visibleHull;	// also clipped to the solid parts of the world
} side_t;


typedef struct bspbrush_s 
{
	struct bspbrush_s*	next;
	struct bspbrush_s*	original;	// chopped up brushes will reference the originals

	int					entitynum;			// editor numbering for messages
	int					brushnum;			// editor numbering for messages

	int					contents;
	bool				opaque;
	int					outputNumber;		// set when the brush is written to the file list

	AABB				bounds;
	int					numsides;
	side_t				sides[6];			// variably sized
} uBrush_t;


typedef struct drawSurfRef_s 
{
	struct drawSurfRef_s*	nextRef;
	int						outputNumber;
} drawSurfRef_t;


typedef struct node_s 
{
	// both leafs and nodes
	int					planenum;	// -1 = leaf node
	struct node_s*		parent;
	AABB				bounds;		// valid after portalization

	// nodes only
	side_t*				side;		// the side that created the node
	struct node_s*		children[2];
	int					nodeNumber;	// set after pruning

	// leafs only
	bool				opaque;		// view can never be inside

	uBrush_t*			brushlist;	// fragments of all brushes in this leaf
	// needed for FindSideForPortal

	int					area;		// determined by flood filling up to areaportals
	int					occupied;	// 1 or greater can reach entity
	uEntity_t*			occupant;	// for leak file testing

	struct uPortal_s *	portals;	// also on nodes during construction
} node_t;


typedef struct uPortal_s 
{
	Planef				plane;
	node_t				*onnode;		// NULL = outside box
	node_t				*nodes[2];		// [0] = front side of plane
	struct uPortal_s	*next[2];
	XWinding			*winding;
} uPortal_t;

// a tree_t is created by FaceBSP()
typedef struct tree_s 
{
	node_t		*headnode;
	node_t		outside_node;
	AABB		bounds;
} tree_t;


typedef struct 
{

} mapLight_t;


typedef struct optimizeGroup_s 
{
	struct optimizeGroup_s	*nextGroup;

	AABB				bounds;			// set in CarveGroupsByLight

	// all of these must match to add a triangle to the triList
	bool				smoothed;				// curves will never merge with brushes
	int					planeNum;
	int					areaNum;
	int					numGroupLights;
	mapLight_t *		groupLights[MAX_GROUP_LIGHTS];	// lights effecting this list
	void *				mergeGroup;		// if this differs (guiSurfs, mirrors, etc), the
	// groups will not be combined into model surfaces
	// after optimization
	textureVectors_t	texVec;

	bool				surfaceEmited;

	mapTri_t *			triList;
	mapTri_t *			regeneratedTris;	// after each island optimization
	Vec3f				axis[2];			// orthogonal to the plane, so optimization can be 2D
} optimizeGroup_t;


inline uBrush_t* AllocBrush(int numsides)
{
	uBrush_t	*bb;
	int			c;

	c = sizeof(uBrush_t)+sizeof(side_t)* (numsides - 6);

	bb = (uBrush_t *)malloc(c);
	memset(bb, 0, c);
	return bb;
}

inline void FreeBrush(uBrush_t* brushes)
{
	int			i;
	for (i = 0; i < brushes->numsides; i++) {
		if (brushes->sides[i].winding) {
			delete brushes->sides[i].winding;
		}
		if (brushes->sides[i].visibleHull) {
			delete brushes->sides[i].visibleHull;
		}
	}
	free(brushes);
}

#endif


typedef core::GrowingStringTableUnique<256, 16, 4, uint32_t> StringTableType;

struct AreaModel
{
	AreaModel();

	bool BelowLimits(void);
	void BeginModel(void);
	void EndModel(void);

	core::Array<model::SubMeshHeader> meshes;
	core::Array<bsp::Vertex> verts;
	core::Array<model::Face> indexes;

	model::MeshHeader model;
};


struct AreaSubMesh
{
	AreaSubMesh() : verts_(g_arena), indexes_(g_arena) {}

	void AddVert(const bsp::Vertex& vert) {
		verts_.append(vert);
	}

	uint32_t matNameID;

	// index's for this sub mesh.
	core::Array<bsp::Vertex> verts_;
	core::Array<model::Face> indexes_;
};

class LvlArea
{
	typedef core::HashMap<core::string, AreaSubMesh> AreaMeshMap;
	typedef core::Array<LvlEntity> AreaEntsArr;
	typedef core::Array<LvlArea> ConnectAreasArr;
public:

	LvlArea() : areaMeshes(g_arena), 
		entities(g_arena), connectedAreas(g_arena) 
	{
		areaMeshes.reserve(2048);
		entities.setGranularity(512);

	}

	void AreaBegin(void);
	void AreaEnd(void);
	AreaSubMesh* MeshForSide(const BspSide& side, StringTableType& stringTable);


public:
	// area has one model.
	AreaModel model;

	AreaMeshMap areaMeshes;
	AreaEntsArr	entities;
	ConnectAreasArr connectedAreas;

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