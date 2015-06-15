#include "stdafx.h"
#include "Level.h"

#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>
#include <IConsole.h>

#include <Math\XWinding.h>

#include <IRenderAux.h>

X_NAMESPACE_BEGIN(level)

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

AreaPortal::AreaPortal()
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

int Level::s_var_usePortals_ = 1;
int Level::s_var_drawAreaBounds_ = 0;
int Level::s_var_drawPortals_ = 0;
int Level::s_var_drawArea_ = -1;
int Level::s_var_drawCurrentAreaOnly_ = 0;

// --------------------------------

Level::Level() :
areas_(g_3dEngineArena),
areaNodes_(g_3dEngineArena),
stringTable_(g_3dEngineArena)
{
	frameID_ = 0;

	canRender_ = false;

	pFileData_ = nullptr;
	pAsyncLoadData_ = nullptr;

	pTimer_ = nullptr;
	pFileSys_ = nullptr;

	core::zero_object(fileHdr_);

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
	
	ADD_CVAR_REF("lvl_drawAreaBounds", s_var_drawAreaBounds_, 0, 0, 1,
		core::VarFlag::SYSTEM, "Draws bounding box around each level area");

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
	if (!canRender())
		return false;

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
				it->pRenderMesh->render();
			}
		}
		else
		{
			if (s_var_drawCurrentAreaOnly_)
			{
				// draw just this area.
				areas_[camArea].pRenderMesh->render();
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
						a.pRenderMesh->render();
					}
				}
			}
		}
	}
	else if (s_var_drawArea_ < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		areas_[s_var_drawArea_].pRenderMesh->render();
	}

	DrawPortalDebug();
	return true;
}

void Level::DrawPortalDebug(void) const
{
	using namespace render;

	if (s_var_drawPortals_ > 0)
	{
		IRenderAux* pAux = gEnv->pRender->GetIRenderAuxGeo();
		XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;

		// 0=off 1=solid 2=wire 3=solid_dt 4=wire_dt
		flags.SetAlphaBlendMode(AuxGeom_AlphaBlendMode::AlphaBlended);

		if (s_var_drawPortals_ == 2 || s_var_drawPortals_ == 4)
		{
			flags.SetFillMode(AuxGeom_FillMode::FillModeWireframe);
		}

		if (s_var_drawPortals_ == 3 || s_var_drawPortals_ == 4)
		{
			flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOn);
			flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOn);
		}
		else
		{
			flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOff);
			flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOff);
		}

		pAux->setRenderFlags(flags);


		// draw the portals.
		AreaArr::ConstIterator areaIt = areas_.begin();
		for (; areaIt != areas_.end(); ++areaIt)
		{
			if (areaIt->frameID != frameID_) {
				continue;
			}

			Area::AreaPortalArr::ConstIterator apIt = areaIt->portals.begin();
			for (; apIt != areaIt->portals.end(); ++apIt)
			{
				const AreaPortal& portal = *apIt;

#if 0
				Vec3f points[20];
				Color8u colors[20];

				int32_t numPoints = portal.pWinding->getNumPoints();
				for (int32_t x = 0; x < numPoints; x++)
				{
					points[x] = (*portal.pWinding)[x].asVec3();

					if (areas_[portal.areaTo].frameID == frameID_) {
						colors[x] = Colorf(0.f, 1.f, 0.f, 0.3f); // visible col.
					}
					else {
						colors[x] = Colorf(1.f, 0.f, 0.f, 0.3f); // visible col.
					}
				}

				pAux->drawTriangle(points, numPoints, colors);
#else

				AABB box;
				portal.pWinding->GetAABB(box);
				
				if (areas_[portal.areaTo].frameID == frameID_)
				{
					pAux->drawAABB(
						box, Vec3f::zero(), true, Colorf(0.f, 1.f, 0.f, 0.45f)
					);
				}
				else
				{
					pAux->drawAABB(
						box, Vec3f::zero(), true, Colorf(1.f, 0.f, 0.f, 1.f)
					);
				}
#endif
			}
		}
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

	while (1)
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
		if (nodeNum < 0) 
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
			if (node.children[1] != 0) 
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
