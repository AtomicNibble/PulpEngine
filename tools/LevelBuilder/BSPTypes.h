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
	bool				allsidesSameMat; // all the sides use same material.
	bool				__pad[2];

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








#endif // X_BSP_TYPES_H_