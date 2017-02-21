#include "stdafx.h"
#include "Level.h"

#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>
#include <IConsole.h>
#include <I3DEngine.h>
#include <IFrameData.h>

#include <Threading\JobSystem2.h>

#include <Math\XWinding.h>
#include "Drawing\PrimativeContext.h"


X_NAMESPACE_BEGIN(level)

// --------------------------------

FrameStats::FrameStats()
{
	clear();
}

void FrameStats::clear(void)
{
	core::zero_this(this);
}

// --------------------------------

AreaNode::AreaNode()
{
	children[0] = -1;
	children[1] = -1;
	commonChildrenArea = -1;
}

// --------------------------------

AreaPortal::AreaPortal() : 
	debugVerts(g_3dEngineArena)
{
	areaTo = -1;
	pWinding = nullptr;
}

AreaPortal::~AreaPortal()
{
	if (pWinding) {
		X_DELETE(pWinding, g_3dEngineArena);
	}
}

// --------------------------------

AreaVisiblePortal::AreaVisiblePortal() :
	visibleEnts(g_3dEngineArena),
	areaFrom(-2)
{
	
}

// --------------------------------

Area::Area() : 
	portals(g_3dEngineArena), 
	visPortals(g_3dEngineArena),
	areaVisibleEnts(g_3dEngineArena),
	cusVisPortalIdx(-1),
	maxVisPortals(0)
{
	areaNum = -1;
	pMesh = nullptr;

	// when loading the level we should set the proper max size
	// so that we never resize post load.
	visPortals.setGranularity(1);
	// visPortals.resize(8);
}

Area::~Area()
{
	pMesh = nullptr; // Area() don't own the memory.


}

void Area::destoryRenderMesh(render::IRender* pRender)
{
	renderMesh.releaseRenderBuffers(pRender);
}


const AABB Area::getBounds(void) const
{
	X_ASSERT_NOT_NULL(pMesh);
	return pMesh->boundingBox;
}


// --------------------------------

PortalStack::PortalStack()
{
	pPortal = nullptr;
	pNext = nullptr;
}

// --------------------------------

Level::AreaRefInfo::AreaRefInfo(core::MemoryArenaBase* arena) :
	areaRefHdrs(arena),
	areaRefs(arena),
	areaMultiRefs(arena)
{

}

void Level::AreaRefInfo::clear(void)
{
	areaRefHdrs.clear();
	areaRefs.clear();
	areaMultiRefs.clear();

	core::zero_object(areaMultiRefHdrs);
}

void Level::AreaRefInfo::free(void)
{
	areaRefHdrs.free();
	areaRefs.free();
	areaMultiRefs.free();

	core::zero_object(areaMultiRefHdrs);
}

// --------------------------------

int Level::s_var_usePortals_ = 1;
int Level::s_var_drawAreaBounds_ = 0;
int Level::s_var_drawPortals_ = 0;
int Level::s_var_drawArea_ = -1;
int Level::s_var_drawCurrentAreaOnly_ = 0;
int Level::s_var_drawStats_ = 0;
int Level::s_var_drawModelBounds_ = 0;
int Level::s_var_drawModelBones_ = 0;
int Level::s_var_drawPortalStacks_ = 0;
int Level::s_var_detechCam_ = 0;
int Level::s_var_cullEnts_ = 0;

// --------------------------------

Level::Level() :
	areas_(g_3dEngineArena),
	areaNodes_(g_3dEngineArena),
	stringTable_(g_3dEngineArena),
	entRefs_(g_3dEngineArena),
	modelRefs_(g_3dEngineArena),
	staticModels_(g_3dEngineArena),
	visibleAreas_(g_3dEngineArena)
{
	outsideWorld_ = true;
	loaded_ = false;
	headerLoaded_ = false;

	pFileData_ = nullptr;

	pTimer_ = nullptr;
	pFileSys_ = nullptr;

	core::zero_object(fileHdr_);
	core::zero_object(visibleAreaFlags_);

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);
	X_ASSERT_NOT_NULL(gEnv->pJobSys);
	X_ASSERT_NOT_NULL(gEnv->pRender);

	pTimer_ = gEnv->pTimer;
	pFileSys_ = gEnv->pFileSys;
	pJobSys_ = gEnv->pJobSys;
	pPrimContex_ = nullptr;
	pScene_ = nullptr;
}

Level::~Level()
{
	free();
}

bool Level::registerVars(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	ADD_CVAR_REF("lvl_usePortals", s_var_usePortals_, 1, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, 
		"Use area portals when rendering the level");
	
	ADD_CVAR_REF("lvl_drawAreaBounds", s_var_drawAreaBounds_, 0, 0, 4,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draws bounding box around each level area. 1=visble 2=all 3=visble-fill 4=all-fill");

	ADD_CVAR_REF("lvl_drawPortals", s_var_drawPortals_, 1, 0, 2, 
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draws the inter area portals. 0=off 1=solid 2=solid_dt");

	ADD_CVAR_REF("lvl_drawArea", s_var_drawArea_, -1, -1, level::MAP_MAX_AREAS,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT, "Draws the selected area index. -1 = disable");

	ADD_CVAR_REF("lvl_drawCurAreaOnly", s_var_drawCurrentAreaOnly_, 0, 0, 1, 
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT, "Draws just the current area. 0=off 1=on");

	ADD_CVAR_REF("lvl_drawStats", s_var_drawStats_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Draws frame stats");

	ADD_CVAR_REF("lvl_drawModelBounds", s_var_drawModelBounds_, 0, 0, 4,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draws bounds around models. 1=visible-AABB 2=visible=Sphere 3=all-AABB 4=all-Sphere");

	ADD_CVAR_REF("lvl_drawModelBones", s_var_drawModelBones_, 0, 0, 4,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draw model bones. 0=off 1=on");

	ADD_CVAR_REF("lvl_drawPortalStacks", s_var_drawPortalStacks_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED, "Draws portal stacks");

	ADD_CVAR_REF("lvl_detachCam", s_var_detechCam_, 0, 0, 2,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT, "Detaches the camera");

	ADD_CVAR_REF("lvl_cullEnts", s_var_cullEnts_, 0, 0, 2,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Perform visibility culling on entities");


	return true;
}

bool Level::init(void)
{
	// get a prim contex to draw all the level debug into :)
	// cast to impl type, for potential de virtulisation..
	pPrimContex_ = static_cast<engine::PrimativeContext*>(get3DEngine()->getPrimContext(engine::PrimContext::MISC3D));


	return true;
}


void Level::free(void)
{
	X_ASSERT_NOT_NULL(pRender_);

	loaded_ = false;

	for (auto& area : areas_) {
		area.destoryRenderMesh(pRender_);
	}

	areas_.free();
	areaNodes_.free();

	entRefs_.free();
	modelRefs_.free();

	staticModels_.free();

	if (pFileData_) {
		X_DELETE_ARRAY(pFileData_, g_3dEngineArena);
		pFileData_ = nullptr;
	}

	if (pScene_) {
		gEnv->pPhysics->releaseScene(pScene_);
		pScene_ = nullptr;
	}
}


bool Level::Load(const char* mapName)
{
	// does the other level file exsist?
	path_.set(mapName);
	path_.setExtension(level::LVL_FILE_EXTENSION);

	// free it :)
	free();

	X_LOG0("Level", "Loading level: %s", mapName);
	loadStats_ = LoadStats();
	loadStats_.startTime = pTimer_->GetTimeNowNoScale();

	// create a physics scene.
	if (!createPhysicsScene()) {
		return false;
	}

	// clear it.
	core::zero_object(fileHdr_);

	headerLoaded_ = false;

	core::IoRequestOpen open;
	open.callback.Bind<Level, &Level::IoRequestCallback>(this);
	open.mode = core::fileMode::READ;
	open.path = path_;

	pFileSys_->AddIoRequestToQue(open);
	return true;
}


void Level::dispatchJobs(core::FrameData& frame)
{
	frameStats_.clear();

	if (s_var_detechCam_ == 0) {
		cam_ = frame.view.cam;
	}


	// here is where we work out if the level is loaded.
	// if it's loaded we want to make visibility jobs.
	// but first we need a job that works out what area we are in.

	if (loaded_)
	{
		core::V2::Job* pSyncJob = pJobSys_->CreateEmtpyJob();

		// find all the visible area's and create lists of all the visible static models in each area.
		auto* pFindVisibleAreas = pJobSys_->CreateMemberJobAsChild<Level>(pSyncJob, this, &Level::FindVisibleArea_job, nullptr);

		auto* pBuildvisFlags = pJobSys_->CreateMemberJobAsChild<Level>(pSyncJob, this, &Level::BuildVisibleAreaFlags_job, nullptr);
		auto* pRemoveDupeVis = pJobSys_->CreateMemberJobAsChild<Level>(pSyncJob, this, &Level::MergeVisibilityArrs_job, nullptr);
		auto* pDrawAreaGeo = pJobSys_->CreateMemberJobAsChild<Level>(pSyncJob, this, &Level::DrawVisibleAreaGeo_job, nullptr);
	//	auto* pDrawStaticModels = pJobSys_->CreateMemberJobAsChild<Level>(pSyncJob, this, &Level::DrawVisibleStaticModels_job, nullptr);

		// now inline build the vis flags for areas
		pJobSys_->AddContinuation(pFindVisibleAreas, pBuildvisFlags, true);
		// then dispatch a job to post proces the visible models, to create single visble lists for each area.
		pJobSys_->AddContinuation(pFindVisibleAreas, pRemoveDupeVis, false);
		// after we have visible areas we can begin drawing the area geo.
		pJobSys_->AddContinuation(pFindVisibleAreas, pDrawAreaGeo, false);
		// after the dupe visibility has been resolved we can start drawing the static models (we could run this after each are'as dupe have been removed)
	//	pJobSys_->AddContinuation(pRemoveDupeVis, pDrawStaticModels, false);


		pJobSys_->Run(pFindVisibleAreas);
		pJobSys_->Run(pSyncJob);

		pJobSys_->Wait(pSyncJob);
		pSyncJob = nullptr;
	}

	// this could be run as a job.
	// but certain debug drawing can be done before others but we can't make multiple jobs
	// since then may not run on same thread.
	// well actually that is fine as long as they don't run in parrallel.
	// so we need to be able to submit debug draw jobs that run at correct time but also ensure two debug jobs can't run at same time
	DrawDebug();
}


void Level::DrawDebug(void)
{
	// we support debug drawing currently it's single threaded.
	// things we can draw:
	// - portals
	// - portal stacks
	// - static model cull results
	// - area bounds

	DebugDraw_AreaBounds();
	DebugDraw_Portals();
	DebugDraw_PortalStacks();
	DebugDraw_StaticModelCullVis();
	DebugDraw_ModelBones();
	DebugDraw_DrawDetachedCam();

}

#if 0
bool Level::render(void)
{
	X_PROFILE_BEGIN("Level render", core::ProfileSubSys::ENGINE3D);

	if (!loaded_) {
		return false;
	}

	clearVisPortals();

	FloodVisibleAreas();

	DrawVisibleAreas();

	DrawMultiAreaModels();
	DrawAreaBounds();
	DrawPortalDebug();
	DrawPortalStacks();
	DrawStatsBlock();

	clearVisableAreaFlags();

	// if the camera is detached always draw the frustum of it.
	if (s_var_detechCam_ > 0) 
	{
		if (s_var_detechCam_ == 1) {
			pPrimContex_->drawFrustum(cam_, Color8u(255, 255, 255, 128), Color8u(200, 0, 0, 100), true);
		}
		else {
			pPrimContex_->drawFrustum(cam_, Color8u(255, 255, 255, 255), Color8u(200, 0, 0, 255), false);
		}
	}

	return true;
}
#endif

#if 0
void Level::DrawArea(const Area& area)
{
	X_ASSERT(IsAreaVisible(area), "Area should be visible when drawing")();

	frameStats_.visibleAreas++;

	const int32_t areaNum = area.areaNum;

	size_t i, end;
	{
		const FileAreaRefHdr& areaModelsHdr = modelRefs_.areaRefHdrs[area.areaNum];

		i = areaModelsHdr.startIndex;
		end = i + areaModelsHdr.num;

		//	X_LOG0("Level", "%i ent refs. start: %i num: %i", area.areaNum, 
		//		areaModelsHdr.startIndex, areaModelsHdr.num);

		for (; i < end; i++)
		{
			uint32_t entId = modelRefs_.areaRefs[i].entId;

			level::StaticModel& model = staticModels_[entId - 1];

			DrawStaticModel(model, areaNum);
		}
	}
	{
		const FileAreaRefHdr& areaEntsHdr = entRefs_.areaRefHdrs[area.areaNum];

		i = areaEntsHdr.startIndex;
		end = i + areaEntsHdr.num;

		for (; i < end; i++)
		{
			uint32_t entId = entRefs_.areaRefs[i].entId;

			X_LOG0("lvl", "VisibleEnt: %i", entId);
		}


		frameStats_.visibleEnts += areaEntsHdr.num;
	}

	frameStats_.visibleVerts += area.pMesh->numVerts;
}

void Level::DrawMultiAreaModels(void)
{
	size_t i, numArea = this->NumAreas();
	size_t numLists = core::Min<size_t>(level::MAP_MAX_MULTI_REF_LISTS,
		(numArea / 32) + 1);

	for (i = 0; i < numLists; i++)
	{
		uint32_t visFlag = visibleAreaFlags_[i];
		const FileAreaRefHdr& refHdr = modelRefs_.areaMultiRefHdrs[i];

		size_t j, num;

		j = refHdr.startIndex;
		num = j + refHdr.num;

		for (; j < num; j++)
		{
			const MultiAreaEntRef& ref = modelRefs_.areaMultiRefs[j];

			// AND them and check for zero.
			uint32_t visible = ref.flags & visFlag;
			if (visible)
			{
				// draw it.
				// ref.entId

			//	level::StaticModel& model = staticModels_[ref.entId - 1];
				
			//	DrawStaticModel(model, 0);
			}
		}
	}
}

bool Level::DrawStaticModel(const level::StaticModel& sm, int32_t areaNum)
{
	using namespace render;
	X_UNUSED(areaNum);

	if (sm.pModel)
	{
		model::XModel* pModel = sm.pModel;

		const Vec3f& pos = sm.pos;
		const Quatf& angle = sm.angle;
		const AABB& wbounds = sm.boundingBox;
		const Sphere& wSphere = sm.boundingSphere;

		Matrix44f posMat = Matrix44f::createTranslation(pos);
		posMat.rotate(angle.getAxis(), angle.getAngle());

		Color8u visColor(255, 255, 64, 128);

		if (s_var_cullEnts_)
		{
			bool culled = false;

			// RED is culled.
			// GREEN is not culled by Sphere but culled by AABB
			// BLUE we are overlapping with sphere

			Color8u cullColor(128, 0, 0, 64);

			if (IsCamArea(areaNum))
			{
				// use the frustrum.		

#if 0
				CullResult::Enum type = cam_.cullAABB_FastT(wbounds);
				if (type == CullResult::EXCLUSION)
				{
					frameStats_.culledModels++;
					culled = true;
				}

#else
				auto type = cam_.cullSphere_ExactT(wSphere);
				if (type == CullResult::OVERLAP)
				{
					if (s_var_cullEnts_ > 1)
					{
						// sphere test says overlap.
						// see if we can cull it with AABB.
						type = cam_.cullAABB_FastT(wbounds);
						if (type == CullResult::EXCLUSION)
						{
							// got culled by AABB
							cullColor = Color8u(0, 128, 0, 128);

							frameStats_.culledModels++;
							culled = true;
						}
					}


					if(!culled)
					{
						// we are visible from overlap.
						visColor = Color8u(0, 0, 128, 128);
					}	
				}
				else if (type == CullResult::EXCLUSION)
				{
					frameStats_.culledModels++;
					culled = true;
				}
#endif
			}
			else
			{
			//	const Area& area = areas_[areaNum];


				// cull it with the portalstack planes.
			//	if (area.CullEnt(wbounds, wSphere))
				{
					frameStats_.culledModels++;
					culled = true;
				}
			}

			if (culled)
			{
				if (s_var_drawModelBounds_ > 2)
				{
					if (s_var_drawModelBounds_ == 3)
					{
						pPrimContex_->drawAABB(sm.boundingBox, false, cullColor);
					}
					else
					{
						pPrimContex_->drawSphere(wSphere, cullColor, false);
					}
				}

				return false;
			}
		}

		frameStats_.visibleModels++;
		frameStats_.visibleVerts += pModel->numVerts(0);

		// we arte rendering this model.
		pModel->Render();
		pModel->RenderBones(pPrimContex_, posMat);

		if (s_var_drawModelBounds_)
		{
			// 1 == aabb 2 = sphee
			if (s_var_drawModelBounds_ == 1 || s_var_drawModelBounds_ == 3)
			{
				pPrimContex_->drawAABB(sm.boundingBox, false, visColor);
			}
			else
			{
				posMat = Matrix44f::createTranslation(pos);
				pPrimContex_->drawSphere(wSphere, visColor, false);
			}
		}

		return true;
	}

	return false;
}
#endif

void Level::clearVisableAreaFlags()
{
	core::zero_object(visibleAreaFlags_);
}

void Level::SetAreaVisible(int32_t areaNum)
{
	size_t index = areaNum / 32;
	uint32_t bit = areaNum % 32;

	visibleAreaFlags_[index] = core::bitUtil::SetBit(
		visibleAreaFlags_[index], bit);
}

bool Level::IsCamArea(int32_t areaNum) const
{
	return areaNum == camArea_;
}

bool Level::IsAreaVisible(int32_t areaNum) const
{
	size_t index = areaNum / 32;
	uint32_t bit = areaNum % 32;

	return core::bitUtil::IsBitSet(
		visibleAreaFlags_[index], bit);
}

bool Level::IsAreaVisible(const Area& area) const
{
	return IsAreaVisible(area.areaNum);
}


size_t Level::NumAreas(void) const
{
	return areas_.size();
}

bool Level::IsPointInAnyArea(const Vec3f& pos) const
{
	int32_t areaOut;
	return IsPointInAnyArea(pos, areaOut);
}

bool Level::IsPointInAnyArea(const Vec3f& pos, int32_t& areaOut) const
{
	if (areaNodes_.isEmpty()) {
		areaOut = -1;
		return false;
	}

	const AreaNode* pNode = &areaNodes_[0];
	int32_t nodeNum;

	X_DISABLE_WARNING(4127)
	while (true)
	X_ENABLE_WARNING(4127)
	{
		float dis = pNode->plane.distance(pos);

		if (dis > 0.f) {
			nodeNum = pNode->children[0];
		}
		else {
			nodeNum = pNode->children[1];
		}
		if (nodeNum == 0) {
			areaOut = -1; // in solid
			return false;
		}
		
		if (nodeNum < 0) 
		{
			nodeNum = (-1 - nodeNum);

			if (nodeNum >= safe_static_cast<int32_t,size_t>(areaNodes_.size())) {
				X_ERROR("Level", "area out of range, when finding point for area");
			}

			areaOut = nodeNum;
			return true;
		}

		pNode = &areaNodes_[nodeNum]; 
	}

	areaOut = -1;
	return false;
}

size_t Level::BoundsInAreas(const AABB& bounds, int32_t* pAreasOut, size_t maxAreas) const
{
	size_t numAreas = 0;

	if (bounds.isEmpty()) {
		X_WARNING("Level","Bounds to areas called with a empty bounds");
		return 0;
	}

	if (maxAreas < 1) {
		X_WARNING("Level","Bounds to areas called with maxarea count of zero");
		return 0;
	}

	// must be valid here.
	X_ASSERT_NOT_NULL(pAreasOut);

	BoundsInAreas_r(0, bounds, numAreas, pAreasOut, maxAreas);

	return numAreas;
}

void Level::BoundsInAreas_r(int32_t nodeNum, const AABB& bounds, size_t& numAreasOut,
	int32_t* pAreasOut, size_t maxAreas) const
{
	X_ASSERT_NOT_NULL(pAreasOut);

	// work out all the areas this bounds intersects with.

	size_t i;

	do 
	{
		if (nodeNum < 0) // negative is a area.
		{
			nodeNum = -1 - nodeNum;

			for (i = 0; i < numAreasOut; i++) {
				if (pAreasOut[i] == nodeNum) {
					break;
				}
			}
			if (i >= numAreasOut && numAreasOut < maxAreas) {
				pAreasOut[numAreasOut++] = nodeNum;
			}

			return;
		}

		const AreaNode& node = areaNodes_[nodeNum];

		PlaneSide::Enum side = bounds.planeSide(node.plane);

		if (side == PlaneSide::FRONT) {
			nodeNum = node.children[0];
		}
		else if (side == PlaneSide::BACK) {
			nodeNum = node.children[1];
		}
		else 
		{
			if (node.children[1] != 0) // 0 is leaf without area since a area of -1 - -1 = 0;
			{
				BoundsInAreas_r(node.children[1], bounds, numAreasOut, pAreasOut, maxAreas);
				if (numAreasOut >= maxAreas) {
					return;
				}
			}
			nodeNum = node.children[0];
		}

	} while (nodeNum != 0);
}

bool Level::createPhysicsScene(void)
{
	if (pScene_) {
		gEnv->pPhysics->releaseScene(pScene_);
	}

	// lets make a scene.
	physics::SceneDesc sceneDesc;
	core::zero_object(sceneDesc.sceneLimitHint); // zero = no limit
	sceneDesc.gravity = Vec3f(0.f, 0.f, -9.8f);
	sceneDesc.frictionType = physics::FrictionType::Patch;
	sceneDesc.frictionOffsetThreshold = 0.04f;
	sceneDesc.contractCorrelationDis = 0.025f;
	sceneDesc.bounceThresholdVelocity = 0.2f;
	sceneDesc.sanityBounds = AABB(Vec3f(static_cast<float>(level::MIN_WORLD_COORD)),
		Vec3f(static_cast<float>(level::MAX_WORLD_COORD)));

	pScene_ = gEnv->pPhysics->createScene(sceneDesc);
	if (!pScene_) {
		return false;
	}

	return true;
}

X_NAMESPACE_END
