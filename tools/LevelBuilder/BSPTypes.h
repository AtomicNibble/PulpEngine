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
//	BspSide*			side;          // the side that created the node 
	struct bspNode*     children[2];
	int					tinyportals;


	// leafs only 
	bool opaque;                    // view can never be inside 
	bool areaportal;
	bool _pad[2];

	int cluster;                   // for portalfile writing 
	int area;                       // for areaportals 


//	bspBrush             *brushlist;     // fragments of all brushes in this leaf 
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


//	bspBrush             *mapBrush;
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