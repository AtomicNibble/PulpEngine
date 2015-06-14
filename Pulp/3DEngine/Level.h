#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include "EngineBase.h"

#include <Ilevel.h>
#include <Time\TimeVal.h>

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
};


struct Area
{
	Area();
	~Area();

public:
	int32_t areaNum;
	// points the the area's mesh header.
	model::MeshHeader* pMesh;
	// the render mesh for the area's model.
	model::IRenderMesh* pRenderMesh;
	// portals leading out this area.
	core::Array<AreaPortal> portals;
};


class Level : public engine::XEngineBase
{
	typedef core::Array<Area> AreaArr;
	typedef core::Array<AreaNode> AreaNodeArr;

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

public:
	// util
	size_t NumAreas(void) const;
	size_t NumPortalsInArea(int32_t areaNum) const;

	bool IsPointInAnyArea(const Vec3f& pos) const;
	bool IsPointInAnyArea(const Vec3f& pos, int32_t& areaOut) const;


private:
	bool ProcessHeader(uint32_t bytesRead);
	bool ProcessData(uint32_t bytesRead);

private:
	int32_t CommonChildrenArea_r(AreaNode* pAreaNode);

private:
	core::GrowingStringTable<256, 16, 4, uint32_t> stringTable_;

	AreaArr areas_;
	AreaNodeArr areaNodes_;

private:
	// pointer to the file data.
	// kept valid while lvl is loaded since mesh headers point to it.
	uint8_t* pFileData_;

	bool canRender_;
	bool _pad[3];

private:
	core::Path path_;
	FileHeader fileHdr_;
	LoadStats loadStats_;
	AsyncLoadData* pAsyncLoadData_;

private:
	core::ITimer* pTimer_;
	core::IFileSys* pFileSys_;

private:
	// vars
	static int s_var_drawAreaBounds_;
	static int s_var_drawPortals_;
	static int s_var_drawArea_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_