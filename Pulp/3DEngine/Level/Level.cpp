#include "stdafx.h"
#include "Level.h"

#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>
#include <IConsole.h>
#include <I3DEngine.h>

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

Area::Area() : 
	portals(g_3dEngineArena), 
	visPortalPlanes(g_3dEngineArena)
{
	areaNum = -1;
	pMesh = nullptr;
}

Area::~Area()
{
	pMesh = nullptr; // Area() don't own the memory.


}

void Area::destoryRenderMesh(render::IRender* pRender)
{
	renderMesh.releaseRenderBuffers(pRender);
}

bool Area::CullEnt(const AABB& wBounds, const Sphere& wSphere) const
{
	X_UNUSED(wBounds);
	X_UNUSED(wSphere);
	// work out if it's visible.
	// the bounds and sphere are world space.

	for (size_t x = 0; x < visPortalPlanes.size(); x++)
	{
		const PortalPlanesArr& portalPlanes = visPortalPlanes[x];

		size_t numPlanes = portalPlanes.size();

		// sphere
		{
			const Vec3f& center = wSphere.center();
			const float radius = -wSphere.radius();

			for (size_t i = 0; i < numPlanes; i++)
			{
				const Planef& plane = portalPlanes[i];

				const float d = plane.distance(center);
				if (d < radius) {
					goto notVisible; // not visible in this stack, might be in another.
				}
			}

			// if we get here we are visible.
			// not culled.
			return false; 
		}

	notVisible:;
	}

	return true; // culled
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
	staticModels_(g_3dEngineArena)
{
	canRender_ = false;
	outsideWorld_ = true;
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
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Draws bounding box around each level area. 1=visble 2=all 3=visble-fill 4=all-fill");

	ADD_CVAR_REF("lvl_drawPortals", s_var_drawPortals_, 1, 0, 4, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Draws the inter area portals. 0=off 1=solid 2=wire 3=solid_dt 4=wire_dt");

	ADD_CVAR_REF("lvl_drawArea", s_var_drawArea_, -1, -1, level::MAP_MAX_AREAS,
		core::VarFlag::SYSTEM, "Draws the selected area index. -1 = disable");

	ADD_CVAR_REF("lvl_drawCurAreaOnly", s_var_drawCurrentAreaOnly_, 0, 0, 1, 
		core::VarFlag::SYSTEM, "Draws just the current area. 0=off 1=on");

	ADD_CVAR_REF("lvl_drawStats", s_var_drawStats_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Draws frame stats");

	ADD_CVAR_REF("lvl_drawModelBounds", s_var_drawModelBounds_, 0, 0, 4,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Draws bounds around models. 1=visible-AABB 2=visible=Sphere 3=all-AABB 4=all-Sphere");

	ADD_CVAR_REF("lvl_drawPortalStacks", s_var_drawPortalStacks_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Draws portal stacks");

	ADD_CVAR_REF("lvl_detachCam", s_var_detechCam_, 0, 0, 2,
		core::VarFlag::SYSTEM, "Detaches the camera");

	ADD_CVAR_REF("lvl_cullEnts", s_var_cullEnts_, 0, 0, 2,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Culls the ent");


	return true;
}

bool Level::init(void)
{
	// get a prim contex to draw all the level debug into :)
	// cast to impl type, for potential de virtulisation..
	pPrimContex_ = static_cast<engine::PrimativeContext*>(get3DEngine()->getPrimContext(engine::PrimContext::MISC3D));



	return true;
}


void Level::update(void)
{
	frameStats_.clear();

	// don't update cam if we are deteched.
	if (!s_var_detechCam_) {
//		cam_ = gEnv->pRender->GetCamera();
	}
}

void Level::free(void)
{
	X_ASSERT_NOT_NULL(pRender_);

	canRender_ = false;

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
}

bool Level::canRender(void)
{
	return canRender_;
}

bool Level::render(void)
{
	X_PROFILE_BEGIN("Level render", core::ProfileSubSys::ENGINE3D);

	if (!canRender()) {
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
#if 0
		render::XAuxGeomRenderFlags flags = render::AuxGeom_Defaults::Def3DRenderflags;
		flags.SetDepthWriteFlag(render::AuxGeom_DepthWrite::DepthWriteOff);
		flags.SetDepthTestFlag(render::AuxGeom_DepthTest::DepthTestOff);
		flags.SetCullMode(render::AuxGeom_CullMode::CullModeNone);
		flags.SetAlphaBlendMode(render::AuxGeom_AlphaBlendMode::AlphaBlended);

		pAux_->setRenderFlags(flags);

		if (s_var_detechCam_ == 1) {
			pAux_->drawFrustum(cam_, Color8u(255, 255, 255, 128), Color8u(200, 0, 0, 100), true);
		}
		else {
			pAux_->drawFrustum(cam_, Color8u(255, 255, 255, 255), Color8u(200, 0, 0, 255), false);
		}
#endif
	}

	return true;
}

void Level::DrawAreaBounds(void)
{
	using namespace render;

	if (s_var_drawAreaBounds_)
	{
		Color color = Col_Red;

#if 1
		if (s_var_drawAreaBounds_ == 1 || s_var_drawAreaBounds_ == 3)
		{
			for (const auto& a : areas_)
			{
				if (IsAreaVisible(a))
				{
					pPrimContex_->drawAABB(a.pMesh->boundingBox, false, color);
				}
			}
		}
		else // all
		{
			for (const auto& a : areas_)
			{
				pPrimContex_->drawAABB(a.pMesh->boundingBox, false, color);
			}
		}




#else
		XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;
		flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOff);
		flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOff);
		pAux_->setRenderFlags(flags);

		// visible only
		if (s_var_drawAreaBounds_ == 1 || s_var_drawAreaBounds_ == 3)
		{
			for (const auto& a : areas_)
			{
				if (IsAreaVisible(a))
				{
					pAux_->drawAABB(a.pMesh->boundingBox, Vec3f::zero(), false, color);
				}
			}
		}
		else // all
		{
			for (const auto& a : areas_)
			{
				pAux_->drawAABB(a.pMesh->boundingBox, Vec3f::zero(), false, color);
			}
		}

		if (s_var_drawAreaBounds_ > 2)
		{
			flags.SetFillMode(AuxGeom_FillMode::FillModeSolid);
			flags.SetAlphaBlendMode(AuxGeom_AlphaBlendMode::AlphaBlended);
			pAux_->setRenderFlags(flags);

			color.a = 0.2f;

			// visible only
			if (s_var_drawAreaBounds_ == 3)
			{
				for (const auto& a : areas_)
				{
					if (IsAreaVisible(a))
					{
						pAux_->drawAABB(a.pMesh->boundingBox, Vec3f::zero(), true, color);
					}
				}
			}
			else // all
			{
				for (const auto& a : areas_)
				{
					pAux_->drawAABB(a.pMesh->boundingBox, Vec3f::zero(), true, color);
				}
			}
		}
#endif
	}
}

void Level::DrawPortalStacks(void) const
{
	if (s_var_drawPortalStacks_)
	{
		// i wanna draw me the planes!
		// we have a clipped shape, described by a collection of planes.
		// how to i turn that into a visible shape.
		// in order to create the shape we need to clip the planes with each other.
		for (const auto& a : areas_)
		{
			if (!IsAreaVisible(a)) {
				continue;
			}

			for (size_t x = 0; x < a.visPortalPlanes.size(); x++)
			{
				const Area::PortalPlanesArr& portalPlanes = a.visPortalPlanes[x];

				XWinding* windings[PortalStack::MAX_PORTAL_PLANES] = { nullptr };
				XWinding* w;

				size_t i, j;
				for (i = 0; i < portalPlanes.size(); i++)
				{
					const Planef& plane = portalPlanes[i];

					w = X_NEW(XWinding, g_3dEngineArena, "PortalStackDebugWinding")(plane);

					for (j = 0; j < portalPlanes.size() && w; j++)
					{
						if (i == j) {
							continue;
						}

						if (!w->clip(portalPlanes[j], 0.01f)) {
							X_DELETE_AND_NULL(w, g_3dEngineArena);
						}
					}

					windings[i] = w;
				}

				// render::XAuxGeomRenderFlags flags = render::AuxGeom_Defaults::Def3DRenderflags;
				// flags.SetDepthWriteFlag(render::AuxGeom_DepthWrite::DepthWriteOff);
				// flags.SetDepthTestFlag(render::AuxGeom_DepthTest::DepthTestOff);
				// pAux_->setRenderFlags(flags);

				Color8u col = Col_Limegreen;

				for (i = 0; i < portalPlanes.size(); i++)
				{
					if (!windings[i]) {
						continue;
					}

					w = windings[i];

					size_t numPoints = w->getNumPoints();
					for (j = 0; j < numPoints; j++)
					{
						Vec3f start = (*w)[j].asVec3();
						Vec3f end = (*w)[(j + 1) % numPoints].asVec3();
						pPrimContex_->drawLine(start, end, col);
					}
				}

				for (i = 0; i < portalPlanes.size(); i++)
				{
					if (!windings[i]) {
						continue;
					}
					X_DELETE(windings[i], g_3dEngineArena);
				}
			}
		}
	}
}

void Level::DrawStatsBlock(void) const
{
	if (!s_var_drawStats_) {
		return;
	}

	// needs re doing with a prim context.
#if 0
	pRender_->Set2D(true);
	{
		core::StackString512 str;

		str.appendFmt("NumAreas:%" PRIuS "\n", areas_.size());
		str.appendFmt("VisibleAreas:%" PRIuS "\n", frameStats_.visibleAreas);
		str.appendFmt("VisibleModels:%" PRIuS "\n", frameStats_.visibleModels);
		str.appendFmt("CulledModel:%" PRIuS "\n", frameStats_.culledModels);
		str.appendFmt("VisibleVerts:%" PRIuS "\n", frameStats_.visibleVerts);
		str.appendFmt("VisibleEnts:%" PRIuS "\n", frameStats_.visibleEnts);
	
		Color txt_col(0.7f, 0.7f, 0.7f, 1.f);
		const float height = 120.f;
		const float width = 200.f;

		float screenWidth = pRender_->getWidthf();

		Vec2f pos(screenWidth - (width + 5.f), 35.f);

		pRender_->DrawQuad(pos.x, pos.y, width, height, Color(0.1f, 0.1f, 0.1f, 0.8f),
			Color(0.01f, 0.01f, 0.01f, 0.95f));
		
		{
			render::DrawTextInfo ti;
			ti.col = txt_col;
			ti.flags = render::DrawTextFlags::POS_2D | render::DrawTextFlags::MONOSPACE;

			{
				Vec3f txtPos(pos.x + 5, pos.y + 20, 1);
				pRender_->DrawText(txtPos, ti, str.c_str());
			}

			{
				Vec3f txtPos(pos.x + (width / 2), pos.y, 1);
				ti.flags |= render::DrawTextFlags::CENTER;
				pRender_->DrawText(txtPos, ti, "Level Draw Stats");
			}
		}
	}
	pRender_->Set2D(false);
#endif
}

void Level::DrawArea(const Area& area)
{
//	if (IsAreaVisible(area)) {
//		return;
//	}

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
		model::XModel* pModel = static_cast<model::XModel*>(sm.pModel);

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
				const Area& area = areas_[areaNum];


				// cull it with the portalstack planes.
				if (area.CullEnt(wbounds, wSphere))
				{
					frameStats_.culledModels++;
					culled = true;
				}
			}

			if (culled)
			{
				if (s_var_drawModelBounds_ > 2)
				{
#if 1

					if (s_var_drawModelBounds_ == 3)
					{
						pPrimContex_->drawAABB(sm.boundingBox, false, cullColor);
					}
					else
					{
						pPrimContex_->drawSphere(wSphere, cullColor, false);
					}

#else
					XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;

					if (s_var_drawModelBounds_ == 3)
					{
						flags.SetCullMode(AuxGeom_CullMode::CullModeNone);
						pAux_->setRenderFlags(flags);
						pAux_->drawAABB(sm.boundingBox, false, cullColor);
					}
					else
					{
						flags.SetFillMode(AuxGeom_FillMode::FillModeWireframe);
						pAux_->setRenderFlags(flags);
						pAux_->drawSphere(wSphere, cullColor, false);
					}
#endif
				}

				return false;
			}
		}

		frameStats_.visibleModels++;
		frameStats_.visibleVerts += pModel->numVerts(0);



//		render::IRender* pRender = getRender();
//		pRender->SetModelMatrix(posMat);

		pModel->Render();
		pModel->RenderBones(pPrimContex_, posMat);

//		pRender->SetModelMatrix(Matrix44f::identity());

		if (s_var_drawModelBounds_)
		{
#if 1

#else
			XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;

			//	flags.SetFillMode(AuxGeom_FillMode::FillModeWireframe);
		//	flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOff);
		//	flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOff);


			// 1 == aabb 2 = sphee
			if (s_var_drawModelBounds_ == 1 || s_var_drawModelBounds_ == 3)
			{
				flags.SetCullMode(AuxGeom_CullMode::CullModeNone);

				pAux_->setRenderFlags(flags);

				pAux_->drawAABB(sm.boundingBox, false, visColor);
			}
			else
			{
				posMat = Matrix44f::createTranslation(pos);

				flags.SetFillMode(AuxGeom_FillMode::FillModeWireframe);
				pAux_->setRenderFlags(flags);

				pAux_->drawSphere(wSphere, visColor, false);
			}
#endif
		}

		return true;
	}

	return false;
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


X_NAMESPACE_END
