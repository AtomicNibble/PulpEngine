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

struct LvlBrush;
struct LvlBrushSide;
struct LvlEntity;

struct bspFace
{
	bspFace();

	struct bspFace* pNext;

	int	planenum;
	bool portal;			
	bool checked;	
	bool _pad[2];

	XWinding* w;
};

struct bspTree;
struct bspNode;

struct bspPortal
{
	bspPortal();

	static bool MakeTreePortals(LvlEntity* ent);

private:
	static void MakeTreePortals_r(bspNode* node);
	static void MakeHeadnodePortals(bspTree& tree);

	// adds the portal to the nodes.
	void AddToNodes(bspNode* front, bspNode* back);
	void RemoveFromNode(bspNode* l);

	// looks for a side on the portal that is a area portal.
	const LvlBrushSide* FindAreaPortalSide(void) const;
	bool HasAreaPortalSide(void) const;

public:
	Planef plane;
	bspNode* onNode;
	bspNode* nodes[2];
	XWinding* pWinding;
	bspPortal* next[2];
};

struct bspNode
{
	friend struct bspTree;

	typedef core::Array<LvlBrush*> lvlBrushArr;

public:
	bspNode();

	void CalcNodeBounds(void);

	XWinding* GetBaseWinding(XPlaneSet& planeSet);

public:
	// leafs and nodes
	int32_t			planenum;			// -1 = leaf node 
	struct bspNode*	parent;
	AABB			bounds;

	// nodes only
	struct bspNode* children[2];

	// leafs only 
	struct bspPortal* portals;

	bool opaque;            // view can never be inside 
	bool _pad[3];

	int32_t area;			// for areaportals 
	int32_t occupied;		// 1 or greater can reach entity 

	lvlBrushArr brushes;


protected:
	void TreePrint_r(const XPlaneSet& planes, size_t depth) const;
};


struct bspTree
{
	bspTree();

	void Print(const XPlaneSet& planes) const;
public:
	bspNode* headnode;
	bspNode	outside_node;
	AABB bounds;
};



X_DECLARE_ENUM(DrawSurfaceType)(FACE,DECAL,PATCH);


struct bspDrawSurface
{
	DrawSurfaceType::Enum type;

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