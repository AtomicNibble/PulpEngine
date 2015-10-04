#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include "EngineBase.h"

#include <Ilevel.h>
#include <Time\TimeVal.h>
#include <array>

#include "String\GrowingStringTable.h"

X_NAMESPACE_DECLARE(model,
struct MeshHeader;
struct IRenderMesh;
)


X_NAMESPACE_BEGIN(level)


struct LoadStats
{
	core::TimeVal startTime;
	core::TimeVal elapse;
};

struct AsyncLoadData
{
	AsyncLoadData(core::XFileAsync* pFile, core::XFileAsyncOperation AsyncOp) :
	headerLoaded_(false),
	waitingForIo_(false),
	pFile_(pFile), 
	AsyncOp_(AsyncOp)
	{}

	~AsyncLoadData();

	bool waitingForIo_;
	bool headerLoaded_;
	core::XFileAsync* pFile_;
	core::XFileAsyncOperation AsyncOp_;
};


struct AreaNode 
{
	static const int32_t CHILDREN_HAVE_MULTIPLE_AREAS = -2;
	static const int32_t AREANUM_SOLID = -1;

public:
	AreaNode();

public:
	Planef	plane;
	// negative numbers are (-1 - areaNumber), 0 = solid
	int32_t	children[2];
	int32_t commonChildrenArea;
};


struct AreaPortal
{
	AreaPortal();
	~AreaPortal();

public:
	int32_t		areaTo;		// the area this portal leads to.
	XWinding*	pWinding;	// winding points have counter clockwise ordering seen this area
	Planef		plane;		// view must be on the positive side of the plane to cross	
	core::Array<Vec3f> debugVerts;
};


struct Area
{
	Area();
	~Area();

public:
	typedef core::Array<AreaPortal> AreaPortalArr;

	int32_t areaNum;
	// points the the area's mesh header.
	model::MeshHeader* pMesh;
	// the render mesh for the area's model.
	model::IRenderMesh* pRenderMesh;
	// portals leading out this area.
	AreaPortalArr portals;

	// used to check if it's visable this frame.
	size_t frameID;
};

struct PortalStack 
{
	static const int MAX_PORTAL_PLANES = 20;
public:
	PortalStack();

public:
	const AreaPortal* pPortal;
	const struct PortalStack* pNext;

	Rectf rect;
	size_t numPortalPlanes;
	Planef portalPlanes[MAX_PORTAL_PLANES + 1];
};


class Level : public engine::XEngineBase
{
	typedef core::Array<Area> AreaArr;
	typedef core::Array<AreaNode> AreaNodeArr;
	typedef core::Array<AreaEntRef> AreaEntRefsArr;
	typedef core::Array<FileAreaRefHdr> AreaEntRefsHdrArr;
	typedef core::Array<MultiAreaEntRef> AreaMultiEntRefsArr;
	typedef std::array<FileAreaRefHdr, MAP_MAX_MULTI_REF_LISTS> AreaMultiEntRefsHdrArr;

public:
	Level();
	~Level();	

	static bool Init(void);
	static void ShutDown(void);

	void update(void);

	void free(void);
	bool canRender(void);
	bool render(void);

	bool Load(const char* mapName);

	void DrawPortalDebug(void) const;

public:
	// util
	size_t NumAreas(void) const;
	size_t NumPortalsInArea(int32_t areaNum) const;

	bool IsPointInAnyArea(const Vec3f& pos) const;
	bool IsPointInAnyArea(const Vec3f& pos, int32_t& areaOut) const;

	size_t BoundsInAreas(const AABB& bounds, int32_t* pAreasOut, size_t maxAreas) const;


private:
	void BoundsInAreas_r(int32_t nodeNum, const AABB& bounds, size_t& numAreasOut,
		int32_t* pAreasOut, size_t maxAreas) const;


	void FlowViewThroughPortals(const int32_t areaNum, const Vec3f origin, 
		size_t numPlanes, const Planef* pPlanes);

	void FloodViewThroughArea_r(const Vec3f origin, int32_t areaNum,
		const PortalStack* ps);

	void AddAreaRefs(int32_t areaNum, const PortalStack* ps);

private:
	bool ProcessHeader(uint32_t bytesRead);
	bool ProcessData(uint32_t bytesRead);

private:
	int32_t CommonChildrenArea_r(AreaNode* pAreaNode);

private:
	core::GrowingStringTable<256, 16, 4, uint32_t> stringTable_;

	AreaArr areas_;
	AreaNodeArr areaNodes_;

	// ent refrences for each area.
	AreaEntRefsHdrArr areaEntRefHdrs_;
	AreaEntRefsArr areaEntRefs_;
	// ent refrences for stuff that is in multiple area's
	AreaMultiEntRefsHdrArr areaEntMultiRefHdrs_;
	AreaMultiEntRefsArr areaMultiEntRefs_;

private:
	size_t frameID_; // inc'd each frame.

	// pointer to the file data.
	// kept valid while lvl is loaded since mesh headers point to it.
	uint8_t* pFileData_;

	bool canRender_;
	bool _pad[3];

private:
	core::Path<char> path_;
	FileHeader fileHdr_;
	LoadStats loadStats_;
	AsyncLoadData* pAsyncLoadData_;

private:
	core::ITimer* pTimer_;
	core::IFileSys* pFileSys_;

private:
	// vars
	static int s_var_usePortals_;
	static int s_var_drawAreaBounds_;
	static int s_var_drawPortals_;
	static int s_var_drawArea_;
	static int s_var_drawCurrentAreaOnly_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_