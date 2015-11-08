#include "stdafx.h"
#include "Level.h"

#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>
#include <IConsole.h>

#include <Math\XWinding.h>
#include <IRenderAux.h>


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

AsyncLoadData::~AsyncLoadData()
{
}

// --------------------------------

AreaNode::AreaNode()
{
	children[0] = -1;
	children[1] = -1;
	commonChildrenArea = -1;
}

// --------------------------------

AreaPortal::AreaPortal() : debugVerts(g_3dEngineArena)
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

Area::Area() : portals(g_3dEngineArena)
{
	areaNum = -1;
	pMesh = nullptr;
	pRenderMesh = nullptr;
}

Area::~Area()
{
	pMesh = nullptr; // Area() don't own the memory.

	if (pRenderMesh) {
		pRenderMesh->release();
	}
}

// --------------------------------

PortalStack::PortalStack()
{
	pPortal = nullptr;
	pNext = nullptr;
	numPortalPlanes = 0;
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

// --------------------------------

Level::Level() :
areas_(g_3dEngineArena),
areaNodes_(g_3dEngineArena),
stringTable_(g_3dEngineArena),
entRefs_(g_3dEngineArena),
modelRefs_(g_3dEngineArena),
staticModels_(g_3dEngineArena)
{
	frameID_ = 0;

	canRender_ = false;

	pFileData_ = nullptr;
	pAsyncLoadData_ = nullptr;

	pTimer_ = nullptr;
	pFileSys_ = nullptr;

	core::zero_object(fileHdr_);
	core::zero_object(visibleAreaFlags_);

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	pTimer_ = gEnv->pTimer;
	pFileSys_ = gEnv->pFileSys;
}

Level::~Level()
{
	free();
}

bool Level::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	ADD_CVAR_REF("lvl_usePortals", s_var_usePortals_, 1, 0, 1,
		core::VarFlag::SYSTEM, "Use area portals when rendering the level.");
	
	ADD_CVAR_REF("lvl_drawAreaBounds", s_var_drawAreaBounds_, 0, 0, 2,
		core::VarFlag::SYSTEM, "Draws bounding box around each level area. 1=visble 2=all");

	ADD_CVAR_REF("lvl_drawPortals", s_var_drawPortals_, 1, 0, 4, core::VarFlag::SYSTEM,
		"Draws the inter area portals. 0=off 1=solid 2=wire 3=solid_dt 4=wire_dt");

	ADD_CVAR_REF("lvl_drawArea", s_var_drawArea_, -1, -1, level::MAP_MAX_AREAS,
		core::VarFlag::SYSTEM, "Draws the selected area index. -1 = disable");

	ADD_CVAR_REF("lvl_drawCurAreaOnly", s_var_drawCurrentAreaOnly_, 0, 0, 1, 
		core::VarFlag::SYSTEM, "Draws just the current area. 0=off 1=on");


	return true;
}

void Level::ShutDown(void)
{


}

void Level::update(void)
{
	frameStats_.clear();

	frameID_++;

	// are we trying to read?
	if (pAsyncLoadData_ && pAsyncLoadData_->waitingForIo_)
	{
		uint32_t numbytes = 0;

		if (pAsyncLoadData_->AsyncOp_.hasFinished(&numbytes))
		{
			pAsyncLoadData_->waitingForIo_ = false;

			if (!pAsyncLoadData_->headerLoaded_)
			{
				pAsyncLoadData_->headerLoaded_ = true;
				ProcessHeader(numbytes);
			}
			else
			{
				// the data is loaded.
				ProcessData(numbytes);
			}
		}
	}
}

void Level::free(void)
{
	canRender_ = false;

#if 0
	// clear render mesh.
	core::Array<AreaModel>::ConstIterator it = areaModels_.begin();
	for (; it != areaModels_.end(); ++it)
	{
		it->pRenderMesh->release();
	}

	areaModels_.free();
#endif

	areas_.free();
	areaNodes_.free();

	entRefs_.free();
	modelRefs_.free();

	staticModels_.free();

	if (pFileData_) {
		X_DELETE_ARRAY(pFileData_, g_3dEngineArena);
		pFileData_ = nullptr;
	}

	if (pAsyncLoadData_) 
	{
		if (pAsyncLoadData_->waitingForIo_)
		{
			// cancel the read request
			pAsyncLoadData_->AsyncOp_.cancel();
			// close file.
			gEnv->pFileSys->closeFileAsync(pAsyncLoadData_->pFile_);
		}

		X_DELETE_AND_NULL(pAsyncLoadData_, g_3dEngineArena);
	}
}

bool Level::canRender(void)
{
	return canRender_;
}

bool Level::render(void)
{
	X_PROFILE_BEGIN("Level render", core::ProfileSubSys::ENGINE3D);

	if (!canRender())
		return false;

	clearVisableAreaFlags();

	// work out what area we are in.
	const XCamera& cam = gEnv->pRender->GetCamera();
	Vec3f camPos = cam.getPosition();

	// build up the cams planes.
	const Planef camPlanes[FrustumPlane::ENUM_COUNT] = {
		-cam.getFrustumPlane(FrustumPlane::LEFT),
		-cam.getFrustumPlane(FrustumPlane::RIGHT),
		-cam.getFrustumPlane(FrustumPlane::TOP),
		-cam.getFrustumPlane(FrustumPlane::BOTTOM),
		-cam.getFrustumPlane(FrustumPlane::FAR),
		-cam.getFrustumPlane(FrustumPlane::NEAR)
	};

	int32_t camArea = -1;
	if (!IsPointInAnyArea(camPos, camArea))
	{
		X_LOG0_EVERY_N(24, "Level", "Outside of world");
	}
	else
	{
		X_LOG0_EVERY_N(24, "Level", "In area: %i", camArea);

#if 0
		const FileAreaRefHdr& areaEnts = areaModelRefHdrs_[camArea];
		const FileAreaRefHdr& multiAreaEnts = areaModelMultiRefHdrs_[0];
		size_t numMulti = 0;
		for (size_t j = 0; j < multiAreaEnts.num; j++)
		{
			if (core::bitUtil::IsBitSet(areaMultiModelRefs_[multiAreaEnts.startIndex + j].flags,
				camArea))
			{
				numMulti++;
			}
		}

		X_LOG0_EVERY_N(24, "Level", "ents In area: %i multi: %i", 
			areaEnts.num, numMulti);
#endif
	}


	AreaArr::ConstIterator it = areas_.begin();
	if (s_var_drawArea_ == -1)
	{
		// if we are outside world draw all.
		// or usePortals is off.
		if (camArea == -1 || s_var_usePortals_ == 0)
		{
			for (; it != areas_.end(); ++it)
			{
				DrawArea(*it); 
			}
		}
		else
		{
			if (s_var_drawCurrentAreaOnly_)
			{
				// draw just this area.
				DrawArea(areas_[camArea]);  
			}
			else
			{
				// we find out all the visable area's.
				// that we can see via portals.
				FlowViewThroughPortals(camArea, camPos, 
					6, camPlanes);

				// for now just render any we touched.
				for (const auto& a : areas_)
				{
					if (a.frameID == frameID_)
					{
						DrawArea(a); 
					}
				}
			}
		}
	}
	else if (s_var_drawArea_ < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		DrawArea(areas_[s_var_drawArea_]);
	}

	// we know all the visible areas now.
	DrawMultiAreaModels();

	DrawAreaBounds();
	DrawPortalDebug();
	DrawStatsBlock();
	return true;
}

void Level::DrawAreaBounds(void)
{
	using namespace render;

	if (s_var_drawAreaBounds_)
	{
		IRenderAux* pAux = gEnv->pRender->GetIRenderAuxGeo();
		XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;
		flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOff);
		flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOff);
		pAux->setRenderFlags(flags);

		Color color = Col_Red;

		// visible only
		if (s_var_drawAreaBounds_ == 1)
		{
			for (const auto& a : areas_)
			{
				if (a.frameID == frameID_)
				{
					pAux->drawAABB(a.pMesh->boundingBox, Vec3f::zero(), false, color);
				}
			}
		}
		else // all
		{
			for (const auto& a : areas_)
			{
				pAux->drawAABB(a.pMesh->boundingBox, Vec3f::zero(), false, color);
			}
		}
	}
}

void Level::DrawStatsBlock(void) const
{
	pRender_->Set2D(true);
	{
		core::StackString512 str;

		str.appendFmt("NumAreas:%i\n", areas_.size());
		str.appendFmt("VisibleAreas:%i\n", frameStats_.visibleAreas);
		str.appendFmt("VisibleModels:%i\n", frameStats_.visibleModels);
	
		Color txt_col(0.7f, 0.7f, 0.7f, 1.f);
		const float height = 100.f;
		const float width = 200.f;

		Vec2f pos(5.f,35.f);

		pRender_->DrawQuad(pos.x, pos.y, width, height, Color(0.1f, 0.1f, 0.1f, 0.8f),
			Color(0.01f, 0.01f, 0.01f, 0.95f));
		
		{
			render::XDrawTextInfo ti;
			ti.col = txt_col;
			ti.flags = render::DrawTextFlags::POS_2D | render::DrawTextFlags::MONOSPACE;

			{
				Vec3f txtPos(pos.x + 5, pos.y + 20, 1);
				pRender_->DrawTextQueued(txtPos, ti, str.c_str());
			}

			{
				Vec3f txtPos(pos.x + (width / 2), pos.y, 1);
				ti.flags |= render::DrawTextFlags::CENTER;
				pRender_->DrawTextQueued(txtPos, ti, "Level Draw Stats");
			}
		}
	}
	pRender_->Set2D(false);
}

void Level::DrawArea(const Area& area)
{
//	if (IsAreaVisible(area)) {
//		return;
//	}

	frameStats_.visibleAreas++;

	SetAreaVisible(area.areaNum);

	const FileAreaRefHdr& areaModelsHdr = modelRefs_.areaRefHdrs[area.areaNum];

	size_t i, end;

	i = areaModelsHdr.startIndex;
	end = i + areaModelsHdr.num;

//	X_LOG0("Level", "%i ent refs. start: %i num: %i", area.areaNum, 
//		areaModelsHdr.startIndex, areaModelsHdr.num);

	for (; i < end; i++)
	{
		uint32_t entId = modelRefs_.areaRefs[i].entId;

		level::StaticModel& model = staticModels_[entId - 1];
		DrawStaticModel(model);
	}

	area.pRenderMesh->render();
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

				level::StaticModel& model = staticModels_[ref.entId - 1];
				
				DrawStaticModel(model);
			}
		}
	}
}

void Level::DrawStaticModel(const level::StaticModel& sm)
{
	if (sm.pModel)
	{
		frameStats_.visibleModels++;

		Vec3f pos = sm.pos;
		Quatf angle = sm.angle;

		Matrix44f posMat = Matrix44f::createTranslation(pos);
		posMat.rotate(angle.getAxis(), angle.getAngle());

		render::IRender* pRender = getRender();
		pRender->SetModelMatrix(posMat);

		sm.pModel->Render();

		pRender->SetModelMatrix(Matrix44f::identity());
	}
}

int32_t Level::CommonChildrenArea_r(AreaNode* pAreaNode)
{
	int32_t	nums[2];

	for ( int i = 0 ; i < 2 ; i++ )
	{
		if (pAreaNode->children[i] <= 0) {
			nums[i] = -1 - pAreaNode->children[i];
		} else {
			nums[i] = CommonChildrenArea_r(&areaNodes_[pAreaNode->children[i]]);
		}
	}

	// solid nodes will match any area
	if (nums[0] == AreaNode::AREANUM_SOLID) {
		nums[0] = nums[1];
	}
	if (nums[1] == AreaNode::AREANUM_SOLID) {
		nums[1] = nums[0];
	}

	int32_t	common;
	if ( nums[0] == nums[1] ) {
		common = nums[0];
	} else {
		common = AreaNode::CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	pAreaNode->commonChildrenArea = common;

	return common;
}

void Level::clearVisableAreaFlags()
{
	core::zero_object(visibleAreaFlags_);
}

void Level::SetAreaVisible(uint32_t area)
{
	size_t index = area / 32;
	uint32_t bit = area % 32;

	visibleAreaFlags_[index] = core::bitUtil::SetBit(
		visibleAreaFlags_[index], bit);
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

bool Level::IsAreaVisible(int32_t areaIdx) const
{
	X_ASSERT((areaIdx >= 0 && areaIdx < safe_static_cast<int, size_t>(areas_.size())),
		"Area index is out of bounds.")(areaIdx, areas_.size());

	return IsAreaVisible(areas_[areaIdx]);
}

bool Level::IsAreaVisible(const Area& area) const
{
	return area.frameID == frameID_;
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


X_NAMESPACE_END
