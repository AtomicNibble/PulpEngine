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

struct FillStats
{
	FillStats();

	void print(void) const;

	size_t numOutside;
	size_t numInside;
	size_t numSolid;
};


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

	static void MakeNodePortal(XPlaneSet& planeSet, bspNode* pNode);
	static bool MakeTreePortals(XPlaneSet& planeSet, LvlEntity* ent);


	// adds the portal to the nodes.
	void AddToNodes(bspNode* pFront, bspNode* pBack);
	void RemoveFromNode(bspNode* pNode);

	// looks for a side on the portal that is a area portal.
	const LvlBrushSide* FindAreaPortalSide(void) const;
	bool HasAreaPortalSide(void) const;

	bool PortalPassable(void) const;

private:
	static void MakeTreePortals_r(XPlaneSet& planeSet, bspNode* pNode);
	static void MakeHeadnodePortals(bspTree& tree);

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
	void MakeTreePortals_r(XPlaneSet& planeSet);
	void FloodPortals_r(int32_t dist, size_t& floodedNum);

	void SplitPortals(XPlaneSet& planes);

	void FillOutside_r(FillStats& stats);

	void ClipSideByTree_r(XPlaneSet& planes, XWinding* w, LvlBrushSide& side);

	void FindAreas_r(size_t& numAreas);
	void FloodAreas_r(size_t areaNum, size_t& areaFloods);

	bool CheckAreas_r(void);

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








#endif // X_BSP_TYPES_H_