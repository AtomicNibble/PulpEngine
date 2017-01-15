#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include "EngineBase.h"

#include <Ilevel.h>
#include <Time\TimeVal.h>
#include <array>

#include "String\GrowingStringTable.h"
#include "Threading\JobList.h"

#include "Model\XModel.h"

X_NAMESPACE_DECLARE(model,
struct MeshHeader;
)


X_NAMESPACE_DECLARE(core,
namespace V2 {
	struct Job;
	class JobSystem;
}
)

X_NAMESPACE_DECLARE(engine,
	class PrimativeContext;
)


X_NAMESPACE_BEGIN(level)


struct LoadStats
{
	core::TimeVal startTime;
	core::TimeVal elapse;
};

struct FrameStats
{
	FrameStats();

	void clear(void);

	size_t culledModels;
	size_t visibleModels;
	size_t visibleAreas;
	size_t visibleVerts;
	size_t visibleEnts;
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


struct PortalStack
{
	static const size_t MAX_PORTAL_PLANES = 20;
	typedef core::FixedArray<Planef, PortalStack::MAX_PORTAL_PLANES + 1> PortalPlanesArr;

public:
	PortalStack();

public:
	const AreaPortal* pPortal;
	const struct PortalStack* pNext;

	Rectf rect;
	PortalPlanesArr portalPlanes;
};

struct Area
{
	typedef core::FixedArray<Planef, PortalStack::MAX_PORTAL_PLANES + 1> PortalPlanesArr;
	typedef core::Array<PortalPlanesArr> VisPortalsArr;
	typedef core::Array<AreaPortal> AreaPortalArr;

public:
	Area();
	~Area();

	void destoryRenderMesh(render::IRender* pRender);
	bool CullEnt(const AABB& bounds, const Sphere& sphere) const;

	const AABB getBounds(void) const;

public:

	int32_t areaNum;
	// points the the area's mesh header.
	model::MeshHeader* pMesh;
	// the gpu buffers for the area mesh.
	model::XRenderMesh renderMesh;
	// portals leading out this area.
	AreaPortalArr portals;
	// plane collections for this area.
	VisPortalsArr visPortalPlanes;
};


class Level : public engine::XEngineBase
{
	typedef core::Array<Area> AreaArr;
	typedef core::Array<AreaNode> AreaNodeArr;
	typedef core::Array<AreaEntRef> AreaRefsArr;
	typedef core::Array<FileAreaRefHdr> AreaRefsHdrArr;
	typedef core::Array<MultiAreaEntRef> AreaMultiRefsArr;
	typedef std::array<FileAreaRefHdr, MAP_MAX_MULTI_REF_LISTS> AreaMultiRefsHdrArr;
	typedef core::Array<level::StaticModel> StaticModelsArr;

	struct AreaRefInfo
	{
		AreaRefInfo(core::MemoryArenaBase* arena);

		void clear(void);
		void free(void);

		// ent with single area ref.
		AreaRefsHdrArr areaRefHdrs;
		AreaRefsArr areaRefs;
		// multi area model refrences for models that are in multiple area's
		AreaMultiRefsHdrArr areaMultiRefHdrs;
		AreaMultiRefsArr areaMultiRefs;
	};

public:
	Level();
	~Level();	

	static bool registerVars(void);

	bool init(void);
	void update(void);

	void free(void);
	bool canRender(void);
	bool render(void);

	bool Load(const char* mapName);

	void DrawPortalDebug(void) const;
	void DrawAreaBounds(void);
	void DrawStatsBlock(void) const;

private:
	void clearVisPortals(void);
	void FloodVisibleAreas(void);
	void DrawVisibleAreas(void);

	void IoRequestCallback(core::IFileSys& fileSys, core::IoRequestData& request,
		core::XFileAsync* pFile, uint32_t bytesTransferred);


	void ProcessHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

public:
	// util
	size_t NumAreas(void) const;
	size_t NumPortalsInArea(int32_t areaNum) const;
	bool AreaHasPortals(int32_t areaNum) const;

	bool IsPointInAnyArea(const Vec3f& pos) const;
	bool IsPointInAnyArea(const Vec3f& pos, int32_t& areaOut) const;

	size_t BoundsInAreas(const AABB& bounds, int32_t* pAreasOut, size_t maxAreas) const;

	bool IsCamArea(int32_t areaNum) const;
	bool IsAreaVisible(int32_t areaNum) const;
	bool IsAreaVisible(const Area& area) const;

private:
	void BoundsInAreas_r(int32_t nodeNum, const AABB& bounds, size_t& numAreasOut,
		int32_t* pAreasOut, size_t maxAreas) const;

	void FlowViewThroughPortals(const int32_t areaNum, const Vec3f origin, 
		size_t numPlanes, const Planef* pPlanes);

	void FloodViewThroughArea_r(const Vec3f origin, int32_t areaNum, const Planef& farPlane,
		const PortalStack* ps);

	void DrawArea(const Area& area);
	void DrawMultiAreaModels(void);
	bool DrawStaticModel(const level::StaticModel& sm, int32_t areaNum);
	void DrawPortalStacks(void) const;

private:
	bool ProcessHeader(void);
	bool ProcessData(void);

private:
	int32_t CommonChildrenArea_r(AreaNode* pAreaNode);

private:
	void clearVisableAreaFlags(void);
	void SetAreaVisible(int32_t area);
	void SetAreaVisible(int32_t areaNum, const PortalStack* ps);

private:
//	core::JobList::JobList jobList_;

private:
	core::GrowingStringTable<256, 16, 4, uint32_t> stringTable_;

	AreaArr areas_;
	AreaNodeArr areaNodes_;

	AreaRefInfo entRefs_;
	AreaRefInfo modelRefs_;

	// static mocel info.
	StaticModelsArr staticModels_;

private:
	FrameStats frameStats_;

private:
	XCamera cam_;

	int32_t camArea_;

	// cleared each frame.
	uint32_t visibleAreaFlags_[MAP_MAX_MULTI_REF_LISTS];

	// pointer to the file data.
	// kept valid while lvl is loaded since mesh headers point to it.
	uint8_t* pFileData_;

	bool canRender_;
	bool outsideWorld_;
	bool headerLoaded_;
	bool _pad[1];

private:
	core::Path<char> path_;
	FileHeader fileHdr_;
	LoadStats loadStats_;


private:
	core::ITimer* pTimer_;
	core::IFileSys* pFileSys_;
	core::V2::JobSystem* pJobSys_;
	engine::PrimativeContext* pPrimContex_;

private:
	// vars
	static int s_var_usePortals_;
	static int s_var_drawAreaBounds_;
	static int s_var_drawPortals_;
	static int s_var_drawArea_;
	static int s_var_drawCurrentAreaOnly_;
	static int s_var_drawStats_;
	static int s_var_drawModelBounds_;
	static int s_var_drawPortalStacks_;
	static int s_var_detechCam_;
	static int s_var_cullEnts_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_