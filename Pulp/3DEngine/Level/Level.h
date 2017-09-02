#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_


#include <Ilevel.h>
#include <Time\TimeVal.h>

#include "String\GrowingStringTable.h"
#include "Threading\JobList.h"

#include "Model\RenderModel.h"

#include <Vars\DrawVars.h>

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

// data for each portal that can see into the area
struct AreaVisiblePortal
{
	typedef core::FixedArray<Planef, PortalStack::MAX_PORTAL_PLANES + 1> PortalPlanesArr;
	typedef core::Array<uint32_t> EntIdArr;
	
public:
	AreaVisiblePortal();

public:
	EntIdArr visibleEnts;	// this list of ent's that are visible in the portal planes below.
	PortalPlanesArr planes; // the planes pointing into area
	int32_t areaFrom; // the area we are coming from
};

struct Area
{
	typedef core::Array<AreaVisiblePortal> VisPortalsArr;
	typedef core::Array<AreaPortal> AreaPortalArr;
	typedef core::Array<uint32_t> EntIdArr;

public:
	Area();
	~Area();

	void destoryRenderMesh(render::IRender* pRender);

	const AABB getBounds(void) const;

public:

	int32_t areaNum;
	// points the the area's mesh header.
	model::MeshHeader* pMesh;
	// the gpu buffers for the area mesh.
	model::XRenderMesh renderMesh;
	// the physics for this area.
	physics::ActorHandle physicsActor;
	// portals leading out this area.
	AreaPortalArr portals;

	// info for portals leading into this area from current camers.
	// when this area is visible, this will contain all the portal planes that point in
	// and also a list of visible ents through said portal.
	int32_t cusVisPortalIdx;
	int32_t maxVisPortals; // this is the max portals that can point into this area
	VisPortalsArr visPortals;

	// processed vis ents.
	EntIdArr areaVisibleEnts;
};


class Level
{
	typedef core::Array<Area> AreaArr;
	typedef core::Array<Area*> AreaPtrArr;
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

	void registerVars(void);

	bool init(void);
	void free(void);

	// free currently level and create jobs for loading new level.
	bool Load(const char* pMapName);

	// created jobs for the level.
	// visibility, culling, rendering
	void dispatchJobs(core::FrameData& frame, render::CommandBucket<uint32_t>& bucket);

private:
	void DrawDebug(void);
	void DebugDraw_AreaBounds(void) const;
	void DebugDraw_Portals(void) const;
	void DebugDraw_PortalStacks(void) const;
	void DebugDraw_StaticModelCullVis(void) const;
	void DebugDraw_ModelBones(void) const;
	void DebugDraw_DrawDetachedCam(void) const;


	// -------- below needs organising ---------
	
	void DrawStatsBlock(void) const;

private:
	void clearVisPortals(void);
	void FloodVisibleAreas(void);
//	void DrawVisibleAreas(void);

	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);


	void ProcessHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
	void FindVisibleArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void FloodThroughPortal_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void FloodViewThroughArea_r(core::V2::Job* pParentJob, const Vec3f origin, int32_t areaNum, int32_t areaFrom, const Planef& farPlane,
		const PortalStack* ps);

	// marks the area visible and creats a job to cull the area's ent's
	void SetAreaVisibleAndCull(core::V2::Job* pParentJob, int32_t area, int32_t areaFrom, const PortalStack* ps = nullptr);

	void CullArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void BuildVisibleAreaFlags_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void MergeVisibilityArrs_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

	void DrawVisibleAreaGeo_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void DrawVisibleStaticModels_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	
	void DrawAreaGeo(Area** pAreas, uint32_t num);
	void DrawStaticModels(const uint32_t* pEntIds, uint32_t num);

	void addMeshTobucket(const model::MeshHeader& mesh, const model::XRenderMesh& renderMesh, const float distanceFromCam);

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

//	void FlowViewThroughPortals(const int32_t areaNum, const Vec3f origin, 
//		size_t numPlanes, const Planef* pPlanes);

	void FloodViewThroughArea_r(const Vec3f origin, int32_t areaNum, const Planef& farPlane,
		const PortalStack* ps);

//	void DrawArea(const Area& area);
//	void DrawMultiAreaModels(void);
//	bool DrawStaticModel(const level::StaticModel& sm, int32_t areaNum);
//	void DrawPortalStacks(void) const;

private:
	bool ProcessHeader(void);
	bool ProcessData(void);

private:
	int32_t CommonChildrenArea_r(AreaNode* pAreaNode);

private:
	void clearVisableAreaFlags(void);
	void SetAreaVisible(int32_t area);
//	void SetAreaVisible(int32_t areaNum, const PortalStack* ps);

private:
	bool createPhysicsScene(void);

private:
	render::CommandBucket<uint32_t>* pBucket_;

private:
	StringTable stringTable_;

	AreaArr areas_;
	AreaNodeArr areaNodes_;

	AreaRefInfo entRefs_;
	AreaRefInfo modelRefs_;

	// static mocel info.
	StaticModelsArr staticModels_;

private:
	FrameStats frameStats_;

	core::Spinlock visAreaLock_;
	AreaPtrArr visibleAreas_;

private:
	XCamera cam_;
	int32_t camArea_;

	// cleared each frame.
	uint32_t visibleAreaFlags_[MAP_MAX_MULTI_REF_LISTS];

	// pointer to the file data.
	// kept valid while lvl is loaded since mesh headers point to it.
	uint8_t* pFileData_;

	bool outsideWorld_;
	bool loaded_;
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
	physics::IScene* pScene_; // the scene for this level.

private:
	engine::DrawVars vars_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_