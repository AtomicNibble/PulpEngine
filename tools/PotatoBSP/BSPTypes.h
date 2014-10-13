#pragma once


#ifndef X_BSP_TYPES_H_
#define X_BSP_TYPES_H_

#include <Containers\LinkedListIntrusive.h>

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

typedef Vec4f textureVectors[2];


struct BspEntity
{
	BspEntity();

	Vec3f					origin;
//	BspPrimitive*			primitives;
	int firstDrawSurf;

	struct bspBrush*		pBrushes;
	struct bspTris*			pPatches;

	mapfile::XMapEntity*	mapEntity;		// points to the map data this was made from.
};

// a brush side, typically 6.
struct BspSide
{
	BspSide() : planenum(0), pWinding(nullptr), pVisibleHull(nullptr) {}

	int				planenum;
	bool			visible;
	bool			culled;

	textureVectors	texVec;

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




class BSPBuilder
{
public:
	BSPBuilder();

	bool LoadFromMap(mapfile::XMapFile* map);
	bool ProcessModels(void);


	const BSPData& getCompiledData(void) {
		return out_;
	}

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
	
	bool processMapEntity(BspEntity& ent, mapfile::XMapEntity* mapEnt);
	bool processBrush(BspEntity& ent, mapfile::XMapBrush* brush, int ent_idx);
	bool processPatch(BspEntity& ent, mapfile::XMapPatch* brush, int ent_idx);

	bool removeDuplicateBrushPlanes(bspBrush* pBrush);

private:

	bool ProcessModel(const BspEntity& ent);
	bool ProcessWorldModel(const BspEntity& ent);


	bspFace* MakeStructuralBspFaceList(bspBrush* list);

	// Face tree building.
	bspTree* FaceBSP(bspFace* list);
	void BuildFaceTree_r(bspNode* node, bspFace* list);

	void SelectSplitPlaneNum(bspNode* node, bspFace* list, int *splitPlaneNum);


	// Filter
	void FilterStructuralBrushesIntoTree(const BspEntity& ent, bspTree* pTree);
	int FilterBrushIntoTree_r(bspBrush* b, bspNode* node);

	// Clip
	void ClipSidesIntoTree(const BspEntity& ent, bspTree* pTree);
	void ClipSideIntoTree_r(XWinding* w, BspSide *side, bspNode *node);

	// bRush Util
	void SplitBrush(bspBrush* brush, int planenum, bspBrush **front, bspBrush **back);

	// Draw Surface
	bspDrawSurface* DrawSurfaceForSide(const BspEntity& ent, bspBrush *b, BspSide *s, XWinding *w);

	void AddEntitySurfaceModels(const BspEntity& ent);
	int AddSurfaceModels(const bspDrawSurface& surface);


	void FilterDrawsurfsIntoTree(const BspEntity& ent, bspTree* pTree);


	static int CountFaceList(bspFace* list);


	int FilterFaceIntoTree(bspDrawSurface* ds, bspTree* pTree);

	void EmitFaceSurface(bspDrawSurface* ds);
	void EmitTriangleSurface(bspDrawSurface* ds);


	void MakeEntityMetaTriangles(const BspEntity& ent);
	void TidyEntitySurfaces(const BspEntity& ent);

	void StripFaceSurface(bspDrawSurface* ds);
	void SurfaceToMetaTriangles(bspDrawSurface* ds);

//	face_t *MakeStructuralBSPFaceList(brush_t *list);

	/*
	int FindFloatPlane(const Planef& plane);
	


	int SelectSplitPlaneNum(node_t* node, bspface_t* list);

	float BrushVolume(uBrush_t* brush);

	void SplitBrush(uBrush_t* brush, int planenum, uBrush_t **front, uBrush_t **back);
	int FilterBrushIntoTree_r(uBrush_t *b, node_t *node);
	void FilterBrushesIntoTree(uEntity_t *e);

	// ---------

	XWinding *BaseWindingForNode(node_t *node);
	void MakeNodePortal(node_t *node);
	void SplitNodePortals(node_t *node);
	void MakeTreePortals_r(node_t *node);
	void MakeTreePortals(tree_t *tree);

	static void RemovePortalFromNode(uPortal_t  *portal, node_t *l);
	static void CalcNodeBounds(node_t *node); 
	static void MakeHeadnodePortals(tree_t *tree);
	static void AddPortalToNodes(uPortal_t  *p, node_t *front, node_t *back);


	// ---------

	bool FloodEntities(tree_t *tree);
	bool PlaceOccupant(node_t* headnode, Vec3f& origin, uEntity_t* occupant);

	static void FloodPortals_r(node_t *node, int dist);

	void FillOutside_r(node_t *node);
	void FillOutside(uEntity_t *e);

	// -------------------------

	void ClipSideByTree_r(XWinding *w, side_t *side, node_t *node);
	void ClipSidesByTree(uEntity_t *e);

	// -------------------------

	void FloodAreas(uEntity_t *e);

	// -------------------------

	void PutPrimitivesInAreas(uEntity_t *e);
	void PutWindingIntoAreas_r(uEntity_t *e, const XWinding *w, side_t *side, node_t *node);
	int CheckWindingInAreas_r(const XWinding *w, node_t *node);
	void ClipTriIntoTree_r(XWinding *w, mapTri_t *originalTri, uEntity_t *e, node_t *node);
	void AddMapTriToAreas(mapTri_t *tri, uEntity_t *e);

	mapTri_t *TriListForSide(const side_t *s, const XWinding *w);
	
	// -------------------------

	void Prelight(uEntity_t *e);
	void CarveGroupsByLight(uEntity_t *e, mapLight_t *light);


	// -------------------------

	void BuildLightShadows(uEntity_t *e, mapLight_t *light);

	// -------------------------

	void FixGlobalTjunctions(uEntity_t *e);
	void FixEntityTjunctions(uEntity_t *e);

	// -------------------------

	void WriteBspMap(primitive_t *list);
	*/

	void EmitDrawVerts(bspDrawSurface* ds, bsp::Surface* out);
	void EmitDrawIndexes(bspDrawSurface* ds, bsp::Surface* out);


private:
	core::Array<BspEntity>		entities;
	core::Array<bspDrawSurface> drawSurfs_;
//	std::vector<mapLight_t*>	mapLights;


	BSPData out_;

	int numSurfacesByType_[DrawSurfaceType::ENUM_COUNT];

	XPlaneSet planes;

	AABB	mapBounds;

	Vec3f blockSize_;

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

// extern MapGlobals gMapGlobals;

#endif // X_BSP_TYPES_H_