#include "stdafx.h"
#include "World3D.h"

#include "Vars\DrawVars.h"
#include "Material\MaterialManager.h"
#include "Model\ModelManager.h"
#include "Drawing\PrimativeContext.h"
#include "Drawing\CBufferManager.h"
#include "Model\RenderModel.h"

#include <Math\XWinding.h>
#include <IFrameData.h>
#include <IRenderMesh.h>
#include <IRenderCommands.h>
#include <CmdBucket.h>

#include <Threading\JobSystem2.h>

#include <Buffer.h>

#include <queue>

X_NAMESPACE_BEGIN(engine)

namespace
{

	bool processIAP(core::XFileFixedBuf& file, Area& area, int32_t areaTo)
	{
		AreaPortal& p = area.portals.AddOne();
		p.pWinding = X_NEW(XWinding, g_3dEngineArena, "AreaPortalWinding");
		p.areaTo = areaTo;
		if (!p.pWinding->SLoad(&file)) {
			X_ERROR("World3D", "Failed to load iap winding");
			X_DELETE(p.pWinding, g_3dEngineArena);
			return false;
		}

		file.readObj(p.plane);

		// read debugVerts.
		uint32_t numVerts;
		if (!file.readObj(numVerts)) {
			X_ERROR("World3D", "Failed to load iap dv's");
			return false;
		}

		p.debugVerts.setGranularity(numVerts);
		p.debugVerts.resize(numVerts);

		// read them.
		if (!file.readObj(p.debugVerts.ptr(), numVerts)) {
			X_ERROR("World3D", "Failed to read iap verts");
			return false;
		}

		return true;
	}


	struct PortalFloodJobData
	{
		PortalStack& basePortalStack;
		const AreaPortal& areaPortal;
		float dis;
		Vec3f camPos;
		int32_t areaFrom;
		const Planef(&camPlanes)[FrustumPlane::ENUM_COUNT];
	};

	struct AreaCullJobData
	{
		int32_t areaIdx;
		int32_t visPortalIdx;
	};


} // namespace 

RenderEnt::RenderEnt(core::MemoryArenaBase* arena) :
	bones(arena)
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
	renderEnts(g_3dEngineArena),
	curVisPortalIdx(-1),
	maxVisPortals(0)
{
	areaNum = -1;
	viewCount = 0;
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


World3D::AreaRefInfo::AreaRefInfo(core::MemoryArenaBase* arena) :
	areaRefHdrs(arena),
	areaRefs(arena),
	areaMultiRefs(arena)
{

}

void World3D::AreaRefInfo::clear(void)
{
	areaRefHdrs.clear();
	areaRefs.clear();
	areaMultiRefs.clear();

	core::zero_object(areaMultiRefHdrs);
}

void World3D::AreaRefInfo::free(void)
{
	areaRefHdrs.free();
	areaRefs.free();
	areaMultiRefs.free();

	core::zero_object(areaMultiRefHdrs);
}

// --------------------------------


World3D::World3D(DrawVars& vars, engine::PrimativeContext* pPrimContex, CBufferManager* pCBufMan,
	physics::IScene* pPhysScene, core::MemoryArenaBase* arena) :
	arena_(X_ASSERT_NOT_NULL(arena)),
	pPhysScene_(X_ASSERT_NOT_NULL(pPhysScene)),
	pPrimContex_(X_ASSERT_NOT_NULL(pPrimContex)),
	pCBufMan_(X_ASSERT_NOT_NULL(pCBufMan)),
	vars_(vars),
	areas_(arena),
	areaNodes_(arena),
	modelRefs_(arena),
	visibleAreas_(arena),
	lights_(arena),
	staticModels_(arena),
	renderEnts_(arena)
{
	viewCount_ = 0;
	frameNumber_ = 0;
	camArea_ = -1;
}

World3D::~World3D()
{


}


void World3D::renderView(core::FrameData& frame, render::CommandBucket<uint32_t>& bucket)
{
	++frameNumber_; // fix me, we might have multiple views.

	X_UNUSED(bucket);
	pBucket_ = &bucket;

	cam_ = frame.view.cam;


	auto* pJobSys = gEnv->pJobSys;

	{
		core::V2::Job* pSyncJob = pJobSys->CreateEmtpyJob(JOB_SYS_SUB_ARG_SINGLE(core::profiler::SubSys::ENGINE3D));

		// find all the visible area's and create lists of all the visible static models in each area.
		auto* pFindVisibleAreas = pJobSys->CreateMemberJobAsChild<World3D>(pSyncJob, this, &World3D::findVisibleArea_job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));

		auto* pBuildvisFlags = pJobSys->CreateMemberJobAsChild<World3D>(pSyncJob, this, &World3D::buildVisibleAreaFlags_job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		auto* pRemoveDupeVis = pJobSys->CreateMemberJobAsChild<World3D>(pSyncJob, this, &World3D::mergeVisibilityArrs_job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		auto* pDrawAreaGeo = pJobSys->CreateMemberJobAsChild<World3D>(pSyncJob, this, &World3D::drawVisibleAreaGeo_job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		auto* pDrawStaticModels = pJobSys->CreateMemberJobAsChild<World3D>(pSyncJob, this, &World3D::drawVisibleStaticModels_job, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));

		// now inline build the vis flags for areas
		pJobSys->AddContinuation(pFindVisibleAreas, pBuildvisFlags, true);
		// then dispatch a job to post proces the visible models, to create single visble lists for each area.
		pJobSys->AddContinuation(pFindVisibleAreas, pRemoveDupeVis, false);
		// after we have visible areas we can begin drawing the area geo.
		pJobSys->AddContinuation(pFindVisibleAreas, pDrawAreaGeo, false);
		// after the dupe visibility has been resolved we can start drawing the static models (we could run this after each are'as dupe have been removed)
		pJobSys->AddContinuation(pRemoveDupeVis, pDrawStaticModels, false);


		pJobSys->Run(pFindVisibleAreas);
		pJobSys->Run(pSyncJob);

		pJobSys->Wait(pSyncJob);
		pSyncJob = nullptr;
	}

	drawRenderEnts();

	drawDebug();
}





bool World3D::loadNodes(const level::FileHeader& fileHdr, level::StringTable& strTable, uint8_t* pData)
{
	auto* pPhys = gEnv->pPhysics;

	areas_.resize(fileHdr.numAreas);

	// area data.
	{
		core::StackString<assetDb::ASSET_NAME_MAX_LENGTH> meshName;
		core::MemCursor cursor(pData + fileHdr.nodes[level::FileNodes::AREA_MODELS].offset,
			fileHdr.nodes[level::FileNodes::AREA_MODELS].size);

		uint32_t x, numSub;

		for (int32_t i = 0; i < fileHdr.numAreas; i++)
		{
			model::MeshHeader* pMesh = cursor.getSeekPtr<model::MeshHeader>();
			numSub = pMesh->numSubMeshes;

			X_ASSERT(numSub > 0, "a areamodel can't have zero meshes")(numSub);

			// set meshHeads verts and faces.
			pMesh->subMeshHeads = cursor.postSeekPtr<model::SubMeshHeader>(numSub);


			// These streams are padded to 16byte align so we must seek past the
			// the padding bytes manually.

			auto seekCursorToPad = [&cursor]()
			{
				// when the value is aligned we must work how much to seek by.
				// we do this by taking a coon to market.
				auto* pCur = cursor.getPtr<const char*>();
				auto* pAligned = core::pointerUtil::AlignTop(pCur, 16);

				const size_t diff = union_cast<size_t>(pAligned) - union_cast<size_t>(pCur);

				cursor.seekBytes(static_cast<uint32_t>(diff));
			};

			seekCursorToPad();
			pMesh->streams[VertexStream::VERT] = cursor.postSeekPtr<uint8_t>(pMesh->numVerts * sizeof(level::VertexBase));

			seekCursorToPad();
			pMesh->streams[VertexStream::COLOR] = cursor.postSeekPtr<Color8u>(pMesh->numVerts);

			seekCursorToPad();
			pMesh->streams[VertexStream::NORMALS] = cursor.postSeekPtr<Vec3f>(pMesh->numVerts);

			// all streams should be 16byte aligned in file post header.
			X_ASSERT_ALIGNMENT(pMesh->streams[VertexStream::VERT].asVoid(), 16, 0);
			X_ASSERT_ALIGNMENT(pMesh->streams[VertexStream::COLOR].asVoid(), 16, 0);
			X_ASSERT_ALIGNMENT(pMesh->streams[VertexStream::NORMALS].asVoid(), 16, 0);

			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->streams[VertexStream::VERT] += pMesh->streams[VertexStream::VERT];
				pSubMesh->streams[VertexStream::COLOR] += pMesh->streams[VertexStream::COLOR];
				pSubMesh->streams[VertexStream::NORMALS] += pMesh->streams[VertexStream::NORMALS];
			}

			seekCursorToPad();

			// indexes
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->indexes = cursor.postSeekPtr<model::Index>(pSubMesh->numIndexes);
			}

			// mat names
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				X_DISABLE_WARNING(4302)
				X_DISABLE_WARNING(4311)
					//		X_ASSERT_NOT_IMPLEMENTED();
				uint32_t matID = reinterpret_cast<uint32_t>(pSubMesh->materialName.as<uint32_t>());

				X_ENABLE_WARNING(4302)
				X_ENABLE_WARNING(4311)

				pSubMesh->materialName = strTable.getString(matID);
				int goat = 0;
				goat = 1;
			}

			// load materials
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];

				pSubMesh->pMat = gEngEnv.pMaterialMan_->loadMaterial(pSubMesh->materialName);
			}

			// set the mesh head pointers.
			pMesh->indexes = pMesh->subMeshHeads[0_sz]->indexes;

			X_ASSERT_ALIGNMENT(pMesh->indexes.asVoid(), 16, 0);

			meshName.clear();
			meshName.appendFmt("$area_mesh%i", i);

			// update area info.
			Area& area = areas_[i];
			area.areaNum = i;
			area.pMesh = pMesh;

			// upload to gpu now.
			// eventually I might delay load them etc.
			area.renderMesh.createRenderBuffers(gEnv->pRender, *area.pMesh, render::shader::VertexFormat::P3F_T4F_C4B_N3F);
		}

		if (!cursor.isEof()) {
			X_WARNING("World3D", "potential read error, cursor is not at end. bytes left: %i",
				cursor.numBytesRemaning());
		}
	}

	// area Portals
	if (fileHdr.flags.IsSet(level::LevelFileFlags::INTER_AREA_INFO))
	{
		core::XFileFixedBuf file = fileHdr.FileBufForNode(pData, level::FileNodes::AREA_PORTALS);

		// 2 ints for the area numbers followed by a winding.
		uint32_t i, numIaps = fileHdr.numinterAreaPortals;

		for (i = 0; i < numIaps; i++)
		{
			int32_t a1, a2;

			file.readObj(a1);
			file.readObj(a2);

			// check the area numbers are valid.
			if (a1 < 0 || a1 > fileHdr.numAreas) {
				X_ERROR("World3D", "Invalid interarea area: %i valid-range(0,%i)", a1, fileHdr.numAreas);
				return false;
			}
			if (a2 < 0 || a2 > fileHdr.numAreas) {
				X_ERROR("World3D", "Invalid interarea area: %i valid-range(0,%i)", a2, fileHdr.numAreas);
				return false;
			}

			// add to areas
			Area& area1 = areas_[a1];
			Area& area2 = areas_[a2];

			++area1.maxVisPortals;
			++area2.maxVisPortals;

			processIAP(file, area1, a2);
			processIAP(file, area2, a1);
		}


		for (auto& a : areas_)
		{
			a.maxVisPortals = core::Max(1, a.maxVisPortals);
			a.visPortals.resize(a.maxVisPortals);
		}

		if (!file.isEof()) {
			X_WARNING("World3D", "potential read error, "
				"failed to process all bytes for node %i. bytes left: %i",
				level::FileNodes::AREA_PORTALS, file.remainingBytes());
		}
	}
	else
	{
		X_WARNING("World3D", "World3D has no inter area portals.");
	}

	// nodes
	if (fileHdr.flags.IsSet(level::LevelFileFlags::BSP_TREE))
	{
		core::XFileFixedBuf file = fileHdr.FileBufForNode(pData, level::FileNodes::BSP_TREE);

		areaNodes_.resize(fileHdr.numNodes);

		// we read: nodeNumber, plane, childid's
		for (int32_t i = 0; i < fileHdr.numNodes; i++)
		{
			AreaNode& node = areaNodes_[i];

			file.readObj(node.plane);
			file.readObj(node.children);
		}

		if (!file.isEof()) {
			X_WARNING("World3D", "potential read error, "
				"failed to process all bytes for node %i. bytes left: %" PRIuS,
				level::FileNodes::BSP_TREE, file.remainingBytes());
		}

		if (areaNodes_.isEmpty())
		{
			X_WARNING("World3D", "World3D has no area nodes");
			return false;
		}

		commonChildrenArea_r(&areaNodes_[0]);
	}
	else
	{
		X_WARNING("World3D", "World3D has no area tree.");
	}


#if 0
	if (fileHdr.flags.IsSet(level::LevelFileFlags::AREA_ENT_REF_LISTS))
	{
		core::XFileFixedBuf file = fileHdr.FileBufForNode(pData, level::FileNodes::AREA_ENT_REFS);

		AreaRefInfo& entRefs = entRefs_;
		entRefs.areaRefHdrs.resize(fileHdr.numAreas);

		file.readObj(entRefs.areaRefHdrs.ptr(), entRefs.areaRefHdrs.size());

		size_t numEntRefs = fileHdr.numEntRefs;
		size_t numMultiAreaEntRefs = fileHdr.numMultiAreaEntRefs;


		// load into single buffer.
		entRefs.areaRefs.resize(numEntRefs);
		file.readObj(entRefs.areaRefs.ptr(), entRefs.areaRefs.size());

		// load multi area ref list headers.
		file.readObj(entRefs.areaMultiRefHdrs.data(), entRefs.areaMultiRefHdrs.size());

		// load the multi area ref lists data.
		entRefs.areaMultiRefs.resize(numMultiAreaEntRefs);
		file.readObj(entRefs.areaMultiRefs.ptr(), entRefs.areaMultiRefs.size());
	}
#endif

	if (fileHdr.flags.IsSet(level::LevelFileFlags::AREA_STATIC_MODEL_REF_LISTS))
	{
		core::XFileFixedBuf file = fileHdr.FileBufForNode(pData, level::FileNodes::AREA_STATIC_MODEL_REFS);

		AreaRefInfo& modelRefs = modelRefs_;
		modelRefs.areaRefHdrs.resize(fileHdr.numAreas);

		file.readObj(modelRefs.areaRefHdrs.ptr(), modelRefs.areaRefHdrs.size());

		size_t numModelRefs = fileHdr.numModelRefs;
		size_t numMultiAreaModelRefs = fileHdr.numMultiAreaModelRefs;


		// load into single buffer.
		modelRefs.areaRefs.resize(numModelRefs);
		file.readObj(modelRefs.areaRefs.ptr(), modelRefs.areaRefs.size());

		// load multi area ref list headers.
		file.readObj(modelRefs.areaMultiRefHdrs.data(), modelRefs.areaMultiRefHdrs.size());

		// load the multi area ref lists data.
		modelRefs.areaMultiRefs.resize(numMultiAreaModelRefs);
		file.readObj(modelRefs.areaMultiRefs.ptr(), modelRefs.areaMultiRefs.size());
	}


	if (fileHdr.flags.IsSet(level::LevelFileFlags::COLLISION))
	{
		core::XFileFixedBuf file = fileHdr.FileBufForNode(pData, level::FileNodes::AREA_COLLISION);

		// add all the area bounds
		for (const auto& a : areas_)
		{
			pPhysScene_->addRegion(a.getBounds());
		}

		physics::IPhysics::DataArr data(g_3dEngineArena);

		for (auto& a : areas_)
		{
			level::AreaCollisionHdr colHdr;
			file.readObj(colHdr);

			Transformf trans;
			trans.pos = colHdr.trans;

			for (uint8_t i = 0; i<colHdr.numGroups; i++)
			{
				level::AreaCollisionGroupHdr groupHdr;
				file.readObj(groupHdr);

				static_assert(level::CollisionDataType::ENUM_COUNT == 4, "Enum count changed? this code may need updating");

				level::AreaCollisionDataHdr dataHdr;

				auto actor = gEnv->pPhysics->createStaticActor(trans, &a);

				for (uint8_t t = 0; t < groupHdr.numTypes[level::CollisionDataType::TriMesh]; t++)
				{
					file.readObj(dataHdr);
					data.resize(dataHdr.dataSize);
					file.read(data.data(), data.size());

					auto triMeshHandle = gEnv->pPhysics->createTriangleMesh(data);
					gEnv->pPhysics->addTriMesh(actor, triMeshHandle);
				}

				for (uint8_t t = 0; t < groupHdr.numTypes[level::CollisionDataType::ConvexMesh]; t++)
				{
					file.readObj(dataHdr);
					data.resize(dataHdr.dataSize);
					file.read(data.data(), data.size());

					auto convexMeshHandle = gEnv->pPhysics->createConvexMesh(data);
					gEnv->pPhysics->addConvexMesh(actor, convexMeshHandle);
				}

				for (uint8_t t = 0; t < groupHdr.numTypes[level::CollisionDataType::HeightField]; t++)
				{
					file.readObj(dataHdr);
					data.resize(dataHdr.dataSize);
					file.read(data.data(), data.size());

					auto hfHandle = gEnv->pPhysics->createHieghtField(data);
					gEnv->pPhysics->addHieghtField(actor, hfHandle);
				}

				AABB aabb;
				for (uint8_t t = 0; t < groupHdr.numTypes[level::CollisionDataType::Aabb]; t++)
				{
					file.readObj(aabb);

					// the AABB is world space right.
					// but the area actor is in area space.
					const Vec3f localPose = aabb.center() - trans.pos;

					gEnv->pPhysics->addBox(actor, aabb, localPose);
				}

				pPhysScene_->addActorToScene(actor);

				a.physicsActor = actor;
			}
		}

	}

	{
		core::XFileFixedBuf file = fileHdr.FileBufForNode(pData, level::FileNodes::STATIC_MODELS);

		staticModels_.resize(fileHdr.numStaticModels);
		if (staticModels_.isNotEmpty())
		{
			// we read each item one by one since we are readong from a FileBuf.
			for (size_t i = 0; i < staticModels_.size(); i++)
			{
				level::StaticModel& sm = staticModels_[i];

				level::FileStaticModel fsm;
				file.readObj(fsm);

				// copy over the info.
				sm.transform.pos = fsm.pos;
				sm.transform.quat = fsm.angle;
				sm.boundingBox = fsm.boundingBox;
				sm.boundingSphere = Sphere(fsm.boundingBox); // create sphere from AABB.
				sm.modelNameIdx = fsm.modelNameIdx;
				// models need to be loaded at some point.
				const char* pModelName = strTable.getString(sm.modelNameIdx);
				model::XModel* pModel = engine::gEngEnv.pModelMan_->loadModel(pModelName);

				sm.pModel = X_ASSERT_NOT_NULL(pModel);
			}

			for (size_t i = 0; i < staticModels_.size(); i++)
			{
				level::StaticModel& sm = staticModels_[i];
				model::XModel* pModel = sm.pModel;

				if (!pModel->isLoaded())
				{
					engine::gEngEnv.pModelMan_->waitForLoad(pModel);
				}

				if (pModel->hasPhys())
				{
					auto actor = pPhys->createStaticActor(sm.transform, pModel);

					pModel->addPhysToActor(actor);

					pPhysScene_->addActorToScene(actor);
				}
			}
		}
	}

	// lights
	{
		core::XFileFixedBuf file = fileHdr.FileBufForNode(pData, level::FileNodes::LIGHTS_STATIC);

		size_t numLights = safe_static_cast<size_t>(file.getSize() / sizeof(level::Light));

		if (numLights)
		{
			lights_.resize(numLights);

			file.readObj(lights_.data(), lights_.size());
		}
	}


	return true;
}

IRenderEnt* World3D::addRenderEnt(RenderEntDesc& entDesc)
{
	X_UNUSED(entDesc);
	X_ASSERT_NOT_NULL(entDesc.pModel);

	auto* pModel = static_cast<model::XModel*>(entDesc.pModel);

	pModel->waitForLoad(gEngEnv.pModelMan_);

	auto* pRenderEnt = X_NEW(RenderEnt, arena_, "RenderEnt")(arena_);
	pRenderEnt->index = safe_static_cast<int32_t>(renderEnts_.size());
	pRenderEnt->lastModifiedFrameNum = 0;
	pRenderEnt->viewCount = 0;
	pRenderEnt->pModel = pModel;
	pRenderEnt->trans = entDesc.trans;

	if (pModel->isAnimated())
	{
//		pRenderEnt->bones.resize(pModel->numBones());


	//	pModel->assingDefaultPose(pRenderEnt->bones.data(), pRenderEnt->bones.size());
	}


	renderEnts_.append(pRenderEnt);

	createEntityRefs(pRenderEnt);

	return pRenderEnt;
}

void World3D::updateRenderEnt(IRenderEnt* pEnt, const Transformf& trans, bool force)
{
	RenderEnt* pRenderEnt = static_cast<RenderEnt*>(pEnt);

	if (!force) {

		// try skipping update.

		if (pRenderEnt->trans == trans) {
			return;
		}
	}

	pRenderEnt->lastModifiedFrameNum = frameNumber_;
	pRenderEnt->trans = trans;

//	createEntityRefs(pRenderEnt);
}

bool World3D::setBonesMatrix(IRenderEnt* pEnt, const Matrix44f* pMats, size_t num)
{
	RenderEnt* pRenderEnt = static_cast<RenderEnt*>(pEnt);
	
	auto* pModel = pRenderEnt->pModel;

	if (!pModel->isAnimated())
	{
		X_ERROR("World", "Can't set blend matrices for none animated mesh");
		return false;
	}

	// you silly slut.
	if (pModel->getNumBones() != static_cast<int32_t>(num))
	{
		X_ERROR("World", "Invalid matrices count for animated mesh");
		return false;
	}

	if (pRenderEnt->bones.isEmpty())
	{
		pRenderEnt->bones.resize(pModel->getNumBones());
	}

	// go from model space to bone space, as we copy.
	const auto& inverseMatrix = pModel->getInverseBoneMatrix();
	X_ASSERT(inverseMatrix.size() == pRenderEnt->bones.size(), "Size mismatch")();
	for (size_t i = 0; i < num; i++)
	{
		pRenderEnt->bones[i] = pMats[i] * inverseMatrix[i];
	}

	return true;
}

IRenderLight* World3D::addRenderLight(RenderLightDesc& ent)
{
	X_UNUSED(ent);

	return nullptr;
}


void World3D::createEntityRefs(RenderEnt* pEnt) 
{
	X_UNUSED(pEnt);

	if (!pEnt->pModel->isLoaded())
	{
		return;
	}

	pEnt->localBounds = pEnt->pModel->bounds();

	if (pEnt->localBounds.isEmpty()) {
		return;
	}

	pEnt->globalBounds = pEnt->localBounds;
	pEnt->globalBounds.move(pEnt->trans.pos);

	viewCount_++;

	pushFrustumIntoTree(pEnt);
}

bool World3D::isPointInAnyArea(const Vec3f& pos, int32_t& areaOut) const
{
	if (areaNodes_.isEmpty()) {
		areaOut = -1;
		return false;
	}

	const AreaNode* pNode = &areaNodes_[0];
	int32_t nodeNum;

	X_DISABLE_WARNING(4127)

	while (true)
	{
		float dis = pNode->plane.distance(pos);

		if (dis > 0.f) {
			nodeNum = pNode->children[level::Side::FRONT];
		}
		else {
			nodeNum = pNode->children[level::Side::BACK];
		}
		if (nodeNum == 0) {
			areaOut = -1; // in solid
			return false;
		}

		if (nodeNum < 0)
		{
			nodeNum = (-1 - nodeNum);

			if (nodeNum >= safe_static_cast<int32_t>(areaNodes_.size())) {
				X_ERROR("Level", "area out of range, when finding point for area");
			}

			areaOut = nodeNum;
			return true;
		}

		pNode = &areaNodes_[nodeNum];
	}

	X_ENABLE_WARNING(4127)

	areaOut = -1;
	return false;
}


size_t World3D::boundsInAreas(const AABB& bounds, int32_t* pAreasOut, size_t maxAreas) const
{
	size_t numAreas = 0;

	if (bounds.isEmpty()) {
		X_WARNING("Level", "Bounds to areas called with a empty bounds");
		return 0;
	}

	if (maxAreas < 1) {
		X_WARNING("Level", "Bounds to areas called with maxarea count of zero");
		return 0;
	}

	// must be valid here.
	X_ASSERT_NOT_NULL(pAreasOut);

	boundsInAreas_r(0, bounds, numAreas, pAreasOut, maxAreas);
	return numAreas;
}

void World3D::boundsInAreas_r(int32_t nodeNum, const AABB& bounds, size_t& numAreasOut,
	int32_t* pAreasOut, size_t maxAreas) const
{
	X_ASSERT_NOT_NULL(pAreasOut);

	// work out all the areas this bounds intersects with.

	do
	{
		if (nodeNum < 0) // negative is a area.
		{
			int32_t areaNum = -1 - nodeNum;
			size_t i;

			for (i = 0; i < numAreasOut; i++) {
				if (pAreasOut[i] == areaNum) {
					break;
				}
			}
			if (i >= numAreasOut && numAreasOut < maxAreas) {
				pAreasOut[numAreasOut++] = areaNum;
			}

			return;
		}

		const AreaNode& node = areaNodes_[nodeNum];

		PlaneSide::Enum side = bounds.planeSide(node.plane);

		if (side == PlaneSide::FRONT) {
			nodeNum = node.children[level::Side::FRONT];
		}
		else if (side == PlaneSide::BACK) {
			nodeNum = node.children[level::Side::BACK];
		}
		else
		{
			if (node.children[level::Side::FRONT] != 0) // 0 is leaf without area since a area of -1 - -1 = 0;
			{
				boundsInAreas_r(node.children[level::Side::FRONT], bounds, numAreasOut, pAreasOut, maxAreas);
				if (numAreasOut >= maxAreas) {
					return;
				}
			}
			nodeNum = node.children[level::Side::BACK];
		}

	} while (nodeNum != 0);
}


void World3D::pushFrustumIntoTree_r(RenderEnt* pEnt, int32_t nodeNum)
{
	do
	{
		if (nodeNum < 0) // negative is a area.
		{
			int32_t areaNum = -1 - nodeNum;
			Area& area = areas_[areaNum];

			// check not already added.
			if (area.viewCount == viewCount_) {
				return;	
			}
			area.viewCount = viewCount_;


			addEntityToArea(area, pEnt);
			return;
		}

		const AreaNode& node = areaNodes_[nodeNum];
		if (node.commonChildrenArea != AreaNode::CHILDREN_HAVE_MULTIPLE_AREAS) {
			// ...
		}


		PlaneSide::Enum side = pEnt->globalBounds.planeSide(node.plane);
		if (side == PlaneSide::FRONT) {
			nodeNum = node.children[level::Side::FRONT];
		}
		else if (side == PlaneSide::BACK) {
			nodeNum = node.children[level::Side::BACK];
		}
		else
		{
			if (node.children[level::Side::FRONT] != 0) // 0 is leaf without area since a area of -1 - -1 = 0;
			{
				pushFrustumIntoTree_r(pEnt, node.children[level::Side::FRONT]);
			}

			nodeNum = node.children[level::Side::BACK];
		}

	} while (nodeNum != 0);
}

void World3D::pushFrustumIntoTree(RenderEnt* pEnt)
{
	pushFrustumIntoTree_r(pEnt, 0);
}

void World3D::addEntityToArea(Area& area, RenderEnt* pEnt)
{

	area.renderEnts.push_back(pEnt);
}


int32_t World3D::commonChildrenArea_r(AreaNode* pAreaNode)
{
	int32_t	nums[2];

	for (int32_t i = 0; i < 2; i++)
	{
		if (pAreaNode->children[i] <= 0) {
			nums[i] = -1 - pAreaNode->children[i];
		}
		else {
			nums[i] = commonChildrenArea_r(&areaNodes_[pAreaNode->children[i]]);
		}
	}

	// solid nodes will match any area
	if (nums[level::Side::FRONT] == AreaNode::AREANUM_SOLID) {
		nums[level::Side::FRONT] = nums[level::Side::BACK];
	}
	if (nums[level::Side::BACK] == AreaNode::AREANUM_SOLID) {
		nums[level::Side::BACK] = nums[level::Side::FRONT];
	}

	int32_t	common;
	if (nums[level::Side::FRONT] == nums[level::Side::BACK]) {
		common = nums[level::Side::FRONT];
	}
	else {
		common = AreaNode::CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	pAreaNode->commonChildrenArea = common;

	return common;
}





void World3D::clearVisPortals(void)
{
	for (auto& a : areas_)
	{
		a.curVisPortalIdx = -1;

		for (auto& vp : a.visPortals)
		{
			vp.visibleEnts.clear();
			vp.planes.clear();
		}
	}
}


void World3D::findVisibleArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	visibleAreas_.clear();

	// clear all active portals from last frame.
	clearVisPortals();

	// we want to find out what area we are in.
	// and then dispatch jobs to recursivly find the others.
	if (vars_.drawArea() == -1)
	{
		const XCamera& cam = cam_;
		const Vec3f camPos = cam.getPosition();

		camArea_ = -1;
		outsideWorld_ = !isPointInAnyArea(camPos, camArea_);


		// if we are outside world draw all.
		// or usePortals is off.
		if (camArea_ < 0 || vars_.usePortals() == 0)
		{
			// add all areas
			for (const auto& area : areas_)
			{
				setAreaVisibleAndCull(pJob, area.areaNum, -1);
			}
		}
		else
		{
			// begin culling this area, while we work out what other area's we can potentially see.
			setAreaVisibleAndCull(pJob, camArea_, -1);

			if (vars_.drawCurrentAreaOnly() != 1)
			{
				// any portals in this area?
				if (!areaHasPortals(camArea_)) {
					// should we exit the game, cus this is likley a box map :|
					return;
				}

				// build up the cams planes.
				const Planef camPlanes[FrustumPlane::ENUM_COUNT] = {
					cam.getFrustumPlane(FrustumPlane::LEFT),
					cam.getFrustumPlane(FrustumPlane::RIGHT),
					cam.getFrustumPlane(FrustumPlane::TOP),
					cam.getFrustumPlane(FrustumPlane::BOTTOM),
					cam.getFrustumPlane(FrustumPlane::FAR),
					cam.getFrustumPlane(FrustumPlane::NEAR)
				};

				const Planef farPlane = cam.getFrustumPlane(FrustumPlane::FAR);

				PortalStack	ps;

				for (size_t i = 0; i < FrustumPlane::ENUM_COUNT; i++) {
					ps.portalPlanes.append(camPlanes[i]);
				}

				// work out what other areas we can see from this one.
				const Area& area = areas_[camArea_];

				auto* pSyncJob = jobSys.CreateEmtpyJob(JOB_SYS_SUB_ARG_SINGLE(core::profiler::SubSys::ENGINE3D));

				for (const auto& portal : area.portals)
				{
					// make sure this portal is facing away from the view (early out)
					float dis = portal.plane.distance(camPos);
					if (dis < -0.0f) {
						continue;
					}

					// now create a job todo the rest of the flooding for this portal
					PortalFloodJobData jobData = { ps, portal, dis, camPos, camArea_, camPlanes };
					auto* pFloodJob = jobSys.CreateMemberJobAsChild<World3D>(pSyncJob, this, &World3D::floodThroughPortal_job, jobData JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
					jobSys.Run(pFloodJob);
				}

				// we must wait for them before leaving this scope.
				jobSys.Run(pSyncJob);
				jobSys.Wait(pSyncJob);
			}
		}
	}
	else if (vars_.drawArea() < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		setAreaVisibleAndCull(pJob, vars_.drawArea(), -1);
	}

}


void World3D::floodThroughPortal_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	const PortalFloodJobData& floodData = *reinterpret_cast<PortalFloodJobData*>(pData);
	// we want to work out if this portal is visible.
	// then travell through it and find all the others.
	const Planef& farPlane = floodData.camPlanes[4];

	if (floodData.dis < 1.0f)
	{
		// it's visible.
		PortalStack	newStack;
		newStack = floodData.basePortalStack;
		newStack.pPortal = &floodData.areaPortal;
		newStack.pNext = &floodData.basePortalStack;
		floodViewThroughArea_r(pJob, floodData.camPos, floodData.areaPortal.areaTo, floodData.areaFrom, farPlane, &newStack);
		return;
	}

	XWinding w;

	// work out if the portal is visible, by clipping the portals winding with that of the cam planes.
	w = (*floodData.areaPortal.pWinding);

	for (uint32_t j = 0; j < FrustumPlane::ENUM_COUNT; j++) {
		if (!w.clipInPlace(floodData.camPlanes[j], 0)) {
			break;
		}
	}
	if (!w.getNumPoints()) {
		return;	// portal not visible
	}

	// go through this portal
	PortalStack	newStack;
	newStack.pPortal = &floodData.areaPortal;
	newStack.pNext = &floodData.basePortalStack;

	Vec3f v1, v2;

	size_t addPlanes = w.getNumPoints();
	if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
		addPlanes = PortalStack::MAX_PORTAL_PLANES;
	}

	for (size_t i = 0; i < addPlanes; i++)
	{
		size_t j = i + 1;
		if (j == w.getNumPoints()) {
			j = 0;
		}

		v1 = floodData.camPos - w[i].asVec3();
		v2 = floodData.camPos - w[j].asVec3();

		Vec3f normal = v2.cross(v1);
		normal.normalize();

		// if it is degenerate, skip the plane
		if (normal.length() < 0.01f) {
			continue;
		}

		Planef& plane = newStack.portalPlanes.AddOne();
		plane.setNormal(normal);
		plane.setDistance(normal * floodData.camPos);
		plane = -plane;
	}

	// far plane
	newStack.portalPlanes.append(farPlane);

	// the last stack plane is the portal plane
	newStack.portalPlanes.append(-floodData.areaPortal.plane);

	floodViewThroughArea_r(pJob, floodData.camPos, floodData.areaPortal.areaTo, floodData.areaFrom, farPlane, &newStack);
}


void World3D::floodViewThroughArea_r(core::V2::Job* pParentJob, const Vec3f origin, int32_t areaNum, int32_t areaFrom,
	const Planef& farPlane, const PortalStack* ps)
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t>(numAreas())),
		"areaNum out of range")(areaNum, numAreas());

	PortalStack	newStack;
	const PortalStack* check;
	XWinding w;
	float dis;
	size_t j;

	const Area& area = areas_[areaNum];

	// add this area.
	setAreaVisibleAndCull(pParentJob, areaNum, areaFrom, ps);

	// see what portals we can see.

	Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
	for (; apIt != area.portals.end(); ++apIt)
	{
		const AreaPortal& portal = *apIt;

		// make sure this portal is facing away from the view
		dis = portal.plane.distance(origin);
		if (dis < -0.0f) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for (check = ps; check; check = check->pNext) {
			if (check->pPortal == &portal) {
				break;	// don't recursively enter a stack
			}
		}
		if (check) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if (dis < 1.0f)
		{
			// go through this portal
			newStack = *ps;
			newStack.pPortal = &portal;
			newStack.pNext = ps;
			floodViewThroughArea_r(pParentJob, origin, portal.areaTo, areaNum, farPlane, &newStack);
			continue;
		}

		// clip the portal winding to all of the planes
		w = *portal.pWinding;

		for (j = 0; j < ps->portalPlanes.size(); j++)
		{
			if (!w.clipInPlace(ps->portalPlanes[j], 0))
			{
				break;
			}
		}
		if (!w.getNumPoints()) {
			continue;	// portal not visible
		}

		// go through this portal
		newStack.pPortal = &portal;
		newStack.pNext = ps;
		newStack.portalPlanes.clear();

		size_t i, addPlanes;
		Vec3f	v1, v2;

		addPlanes = w.getNumPoints();
		if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
			addPlanes = PortalStack::MAX_PORTAL_PLANES;
		}

		for (i = 0; i < addPlanes; i++)
		{
			j = i + 1;
			if (j == w.getNumPoints()) {
				j = 0;
			}

			v1 = origin - w[i].asVec3();
			v2 = origin - w[j].asVec3();

			Vec3f normal = v2.cross(v1);
			normal.normalize();

			// if it is degenerate, skip the plane
			if (normal.length() < 0.01f) {
				continue;
			}

			Planef& plane = newStack.portalPlanes.AddOne();
			plane.setNormal(normal);
			plane.setDistance(normal * origin);
			plane = -plane;
		}

		newStack.portalPlanes.append(farPlane);

		// the last stack plane is the portal plane
		newStack.portalPlanes.append(-portal.plane);

		floodViewThroughArea_r(pParentJob, origin, portal.areaTo, areaNum, farPlane, &newStack);
	}
}

void World3D::setAreaVisible(int32_t areaNum)
{
	uint32_t index = areaNum / 32;
	uint32_t bit = areaNum % 32;

	visibleAreaFlags_[index] = core::bitUtil::SetBit(visibleAreaFlags_[index], bit);
}

void World3D::setAreaVisible(int32_t areaNum, int32_t areaFrom, const PortalStack* ps)
{
	if (areaNum == camArea_ && areaNum >= 0) {
		X_WARNING("Level", "Portal flood ended back in cam area skipping visibility for stack");
		return;
	}

	Area& area = areas_[areaNum];

	setAreaVisible(areaNum);

	int32_t visPortalIdx = -1;

	if (area.maxVisPortals)
	{
		visPortalIdx = core::atomic::Increment(&area.curVisPortalIdx);

		X_ASSERT(visPortalIdx < area.maxVisPortals, "Area entered from more portals than expected")(areaNum, areaFrom, visPortalIdx, area.maxVisPortals, ps);

		area.visPortals[visPortalIdx].areaFrom = areaFrom;

		if (ps) {
			area.visPortals[visPortalIdx].planes = ps->portalPlanes;
		}
	}
}


void World3D::setAreaVisibleAndCull(core::V2::Job* pParentJob, int32_t areaNum, int32_t areaFrom, const PortalStack* ps)
{
	if (areaNum == camArea_ && areaFrom >= 0) {
		X_WARNING("Level", "Portal flood ended back in cam area skipping visibility for stack");
		return;
	}

	Area& area = areas_[areaNum];

	{
		core::Spinlock::ScopedLock lock(visAreaLock_);
		if (std::find(visibleAreas_.begin(), visibleAreas_.end(), &area) == visibleAreas_.end()) {
			visibleAreas_.push_back(&area);
		}
	}

	int32_t visPortalIdx = -1;

	if (area.maxVisPortals)
	{
		visPortalIdx = core::atomic::Increment(&area.curVisPortalIdx);

		X_ASSERT(visPortalIdx < area.maxVisPortals, "Area entered from more portals than expected")(areaNum, areaFrom, visPortalIdx, area.maxVisPortals, ps);

		area.visPortals[visPortalIdx].areaFrom = areaFrom;

		if (ps) {
			// this is thread safe as each thread gets a diffrent idx.
			area.visPortals[visPortalIdx].planes = ps->portalPlanes;
		}
	}

	AreaCullJobData data = { areaNum, visPortalIdx };
	auto* pJob = gEnv->pJobSys->CreateMemberJobAsChild<World3D>(pParentJob, this, &World3D::cullArea_job, data JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));

	gEnv->pJobSys->Run(pJob);
}



void World3D::cullArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	const AreaCullJobData& jobData = *reinterpret_cast<AreaCullJobData*>(pData);

	Area& area = areas_[jobData.areaIdx];

	const level::FileAreaRefHdr& areaModelsHdr = modelRefs_.areaRefHdrs[jobData.areaIdx];
	size_t i = areaModelsHdr.startIndex;
	size_t end = i + areaModelsHdr.num;


	if (jobData.visPortalIdx < 0)
	{
		return;
	}

	auto& visPortal = area.visPortals[jobData.visPortalIdx];

	visPortal.visibleEnts.clear();

	// we want to cull everything in this area.
	// it's either culled with the camera or the portal planes.
	if (visPortal.planes.isEmpty())
	{
		// process all the static models in this area
		{
			for (; i < end; i++)
			{
				uint32_t modelId = modelRefs_.areaRefs[i].modelId;
				const level::StaticModel& sm = staticModels_[modelId];

				auto type = cam_.cullSphere_ExactT(sm.boundingSphere);
				if (type == CullResult::OVERLAP)
				{
					if (vars_.cullEnts() > 1)
					{
						// sphere test says overlap.
						// see if we can cull it with AABB.
						type = cam_.cullAABB_FastT(sm.boundingBox);
						if (type == CullResult::EXCLUSION)
						{
							// got culled by AABB
							continue;
						}
					}
				}
				else if (type == CullResult::EXCLUSION)
				{
			//		frameStats_.culledModels++;
					continue;
				}

				// this model is visible.
				visPortal.visibleEnts.emplace_back(modelId);
			}
		}
	}
	else
	{
		// get the portal planes we are checking against.
		const auto& planes = visPortal.planes;

		// process all the static models in this area
		{
			for (; i < end; i++)
			{
				uint32_t modelId = modelRefs_.areaRefs[i].modelId;
				const level::StaticModel& sm = staticModels_[modelId];

				// sphere check
				const Vec3f& center = sm.boundingSphere.center();
				const float radius = -sm.boundingSphere.radius();

				for (const auto& plane : planes)
				{
					const float d = plane.distance(center);
					if (d < radius) {
						continue; // not visible in this stack, might be in another.
					}
				}

				// AABB check?
				// ...


				// if we get here we are visible.
				visPortal.visibleEnts.emplace_back(modelId);
			}
		}
	}

	// sort them.
	std::sort(visPortal.visibleEnts.begin(), visPortal.visibleEnts.end());

	// we need to do stuff..

}



void World3D::buildVisibleAreaFlags_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	for (auto* pArea : visibleAreas_)
	{
		setAreaVisible(pArea->areaNum);
	}
}


void World3D::mergeVisibilityArrs_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	// ok so all the visible area's have been found and models have been culled.
	// we just need to do some post process on the cull reuslts.
	// since we have seperate cull lists for each portal that enters a area.
	// 

	for (auto* pArea : visibleAreas_)
	{
		if (pArea->curVisPortalIdx > 0)
		{
			const size_t total = core::accumulate(pArea->visPortals.begin(), pArea->visPortals.begin() + pArea->curVisPortalIdx + 1, 0_sz, [](const AreaVisiblePortal& avp) {
				return avp.visibleEnts.size();
			});

			// make engouth space for them all
			pArea->areaVisibleEnts.reserve(total);
			pArea->areaVisibleEnts.clear();

			// create a que that gives us the smallets lists first.
			auto cmp = [](const AreaVisiblePortal::EntIdArr* lhs, const AreaVisiblePortal::EntIdArr* rhs) { return lhs->size() < rhs->size(); };
			std::priority_queue<const AreaVisiblePortal::EntIdArr*, core::FixedArray<const AreaVisiblePortal::EntIdArr*, 16>, decltype(cmp)> que(cmp);

			for (int32_t i = 0; i < pArea->curVisPortalIdx + 1; i++)
			{
				const auto& visEnts = pArea->visPortals[i].visibleEnts;
#if X_DEBUG
				X_ASSERT(std::is_sorted(visEnts.begin(), visEnts.end()), "Not sorted")();
#endif // X_DEBUG
				que.push(&visEnts);
			}

			X_ASSERT(que.size() >= 2, "source code error should be atleast 2 in que")(que.size());

			auto& destArr = pArea->areaVisibleEnts;

			// copy first two into dest then merge.
			auto* pVisEnts1 = que.top();
			que.pop();
			std::copy(pVisEnts1->begin(), pVisEnts1->end(), std::back_inserter(destArr));

			auto mergepoint = destArr.end();
			auto* pVisEnts2 = que.top();
			que.pop();
			std::copy(pVisEnts2->begin(), pVisEnts2->end(), std::back_inserter(destArr));

			std::inplace_merge(destArr.begin(), mergepoint, destArr.end());

			// remove dupes
			auto pte = std::unique(destArr.begin(), destArr.end());
			destArr.erase(pte, destArr.end());

			// now we need to just keep copying and merging the others.
			while (!que.empty())
			{
				auto pVisEnts = que.top();
				que.pop();

				mergepoint = destArr.end();
				std::copy(pVisEnts->begin(), pVisEnts->end(), std::back_inserter(destArr));
				std::inplace_merge(destArr.begin(), mergepoint, destArr.end());

				// remove dupes
				pte = std::unique(destArr.begin(), destArr.end());
				destArr.erase(pte, destArr.end());
			}
		}
		else
		{
			pArea->areaVisibleEnts.swap(pArea->visPortals[0].visibleEnts);
		}
	}
}

void World3D::drawVisibleAreaGeo_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	// here we add draw calls for the visible area's to cmdBucket.
	// 
	core::Delegate<void(Area**, uint32_t)> del;
	del.Bind<World3D, &World3D::drawAreaGeo>(this);

	auto* pJobs = jobSys.parallel_for_member_child<World3D, Area*>(pJob, del, visibleAreas_.data(),
		safe_static_cast<uint32_t>(visibleAreas_.size()), core::V2::CountSplitter32(1) JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));

	jobSys.Run(pJobs);
}

void World3D::drawVisibleStaticModels_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	// we just dispatch parallel jobs for each area.
	// this will then create multiple jobs for each area
	// depending on how many ents are in the area

	core::Delegate<void(const uint32_t*, uint32_t)> del;
	del.Bind<World3D, &World3D::drawStaticModels>(this);

	for (auto* pArea : visibleAreas_)
	{
		const auto& visEnts = pArea->areaVisibleEnts;

		if (visEnts.isNotEmpty())
		{
			auto* pJobs = jobSys.parallel_for_member_child<World3D>(pJob, del, visEnts.data(), safe_static_cast<uint32_t>(visEnts.size()),
				core::V2::CountSplitter32(16) // will likley need tweaking, props even made a var.
				JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D)
				);

			jobSys.Run(pJobs);
		}
	}
}


void World3D::drawAreaGeo(Area** pAreas, uint32_t num)
{
	Matrix44f world = Matrix44f::identity();

	for (uint32_t i = 0; i < num; i++)
	{
		Area* pArea = pAreas[i];

		const model::MeshHeader& mesh = *pArea->pMesh;
		const auto& renderMesh = pArea->renderMesh;
		const float distanceFromCam = 0; // humm

		addMeshTobucket(mesh, renderMesh, render::shader::VertexFormat::P3F_T4F_C4B_N3F, world, distanceFromCam);

	}
}

void World3D::drawStaticModels(const uint32_t* pModelIds, uint32_t num)
{
	X_UNUSED(pModelIds, num);

	for (uint32_t i = 0; i < num; i++)
	{
		const auto modelId = pModelIds[i];
		level::StaticModel& sm = staticModels_[modelId];

		// should this be allowed?
		if (!sm.pModel) {
			continue;
		}

		model::RenderModel* pModel = static_cast<model::RenderModel*>(sm.pModel);

		// is this a good place todo the bucketing?
		// like placing the model in multiple buckets
		// seams like a good palce since we are fully parralel here.
		// should we also handle lod logic here?
		// how to handle instancing?
		// since if we have 10 models that are the same we want to draw them as a batch.
		// for static models we can work out models that are the same so for a visible area we have the info about what can be instanced.
		// i think per area batching should work well enougth in practice tho.
		// then that would need processing / culling a little diffrent so that i end up with list of ents for instanced drawing
		// which can then have buffers made with all the instanced info and populate each frame and draw.
		// i don't think pre making the instanced buffers per a area is worth it as it will make culling harder.
		// so for now we just handle the rendering of a model.
		size_t lodIdx = 0;

		const float distanceFromCam = cam_.getPosition().distance(sm.transform.pos);

		if (pModel->hasLods())
		{
			// work out which lod to select based on distance.
			lodIdx = pModel->lodIdxForDistance(distanceFromCam);

			// now just because we want to render with this lod don't mean we can
			// for example we may have not uploaded the mesh for this lod to the gpu

			if (!pModel->canRenderLod(lodIdx))
			{
				// lets try pick another lod.
				// higer or lower?
				while (lodIdx > 0 && !pModel->canRenderLod(lodIdx))
				{
					--lodIdx;
				}
			}
		}

		// upload if needed.
		if (!pModel->canRenderLod(lodIdx))
		{
			pModel->createRenderBuffersForLod(lodIdx, gEnv->pRender);
		}

		const model::MeshHeader& mesh = pModel->getLodMeshHdr(lodIdx);
		const auto& renderMesh = pModel->getLodRenderMesh(lodIdx);


#if 1
		Matrix44f world = Matrix44f::createTranslation(sm.transform.pos);
		
		if (sm.transform.quat.getAngle() != 0.f) {
			world.rotate(sm.transform.quat.getAxis(), sm.transform.quat.getAngle());
		}
#else

		Matrix44f world = sm.angle.toMatrix44();
		world.setTranslate(sm.pos);
#endif

#if 0
		if (pModel->numBones())
		{
			if (vars_.drawModelBones())
			{
				pModel->RenderBones(pPrimContex_, world, vars_.boneColor());
			}

			if (vars_.drawModelBoneNames())
			{
				Matrix33f view = cam_.getViewMatrix().subMatrix33(0,0);
				view.rotate(Vec3f::yAxis(), ::toRadians(180.f));
				view.rotate(Vec3f::zAxis(), ::toRadians(180.f));

				pModel->RenderBoneNames(pPrimContex_, world, view, vars_.boneNameOffset(), vars_.boneNameSize(), vars_.boneNameColor());
			}
		}
#endif

	//	if (vars_.drawModelBounds())
	//	{
	//		pPrimContex_->drawAABB(sm.boundingBox, false, Col_Orangered);
	//	}

		world.transpose();

		addMeshTobucket(mesh, renderMesh, render::shader::VertexFormat::P3F_T2S_C4B_N3F, world, distanceFromCam);
	}
}


void World3D::drawRenderEnts()
{
	for (auto* pRendEnt : renderEnts_)
	{
		const float distanceFromCam = cam_.getPosition().distance(pRendEnt->trans.pos);

		Matrix44f world = Matrix44f::createTranslation(pRendEnt->trans.pos);

		if (pRendEnt->trans.quat.getAngle() != 0.f) {
			world.rotate(pRendEnt->trans.quat.getAxis(), pRendEnt->trans.quat.getAngle());
		}


		model::RenderModel* pModel = static_cast<model::RenderModel*>(pRendEnt->pModel);
		size_t lodIdx = 0;

		if (!pModel->isLoaded())
		{
			continue;
		}

		if (!pModel->canRenderLod(lodIdx))
		{
			pModel->createRenderBuffersForLod(lodIdx, gEnv->pRender);
		}

		bool hasBones = pRendEnt->bones.isNotEmpty();

		const model::MeshHeader& mesh = pModel->getLodMeshHdr(lodIdx);
		const auto& renderMesh = pModel->getLodRenderMesh(lodIdx);

		if (hasBones && !renderMesh.hasVBStream(VertexStream::HWSKIN))
		{
			pModel->createSkinningRenderBuffersForLod(lodIdx, gEnv->pRender);
		}

		// Debug drawing.
		if (hasBones)
		{
			if (vars_.drawModelBones())
			{
				pModel->RenderBones(pPrimContex_, world, vars_.boneColor(), pRendEnt->bones.data(), pRendEnt->bones.size());
			}

			if (vars_.drawModelBoneNames())
			{
				Matrix33f view = cam_.getViewMatrix().subMatrix33(0, 0);
				view.rotate(Vec3f::yAxis(), ::toRadians(180.f));
				view.rotate(Vec3f::zAxis(), ::toRadians(180.f));
		
				pModel->RenderBoneNames(pPrimContex_, world, view, vars_.boneNameOffset(), vars_.boneNameSize(), 
					vars_.boneNameColor(), pRendEnt->bones.data(), pRendEnt->bones.size());
			}
		}

		if (vars_.drawModelBounds())
		{
			OBB obb(pRendEnt->trans.quat, pRendEnt->localBounds);

			pPrimContex_->drawOBB(obb, pRendEnt->trans.pos, false, Col_Orangered);
		}
		// ~Debug

#if 0
		auto& bones = pRendEnt->bones;

		if (bones.size() > 8)
		{


			static float counter = 0.f;
			static bool forward = true;

			//	static RenderEnt::MatrixArr bones(g_3dEngineArena, 10);

				//	bones[0].setTranslate(Vec3f(0.f, 0.f, counter / 3));
			bones[1].setTranslate(Vec3f(0.f, 0.f, counter / 4.f));
			bones[2] = Matrix44f::createRotation(Vec3f(0.f, 1.f, 0.f), counter / 100.f);
			//	bones[3].setTranslate(Vec3f(0.f, 0.f, counter / 4.f));

				// bones[4].setTranslate(Vec3f(0.f, 0.f, counter));
			bones[5].setTranslate(Vec3f(0.f, counter / 2.f, 0.f));
			bones[6].setTranslate(Vec3f(0.f, counter / 1.5f, 0.f));

			bones[8].setTranslate(Vec3f(counter / 4.f, 0.f, 0.f));

			if (forward)
			{
				counter += 0.1f;

				if (counter >= 0.f)
				{
					forward = false;
				}
			}
			else
			{
				counter -= 0.1f;

				if (counter < -20.f)
				{
					forward = true;
				}
			}
		}

	//	for (size_t i = 0; i < bones.size(); i++)
	//	{
	//		pRendEnt->bones[i] = bones[i];
	//	}	

#endif

		if (hasBones)
		{
			auto* pBoneDataDes = pBucket_->createDynamicBufferDesc();
			pBoneDataDes->pData = pRendEnt->bones.data();
			pBoneDataDes->size = safe_static_cast<uint32_t>(pRendEnt->bones.size() * sizeof(Matrix44f));


			world.transpose();


			addMeshTobucket(mesh, renderMesh, render::shader::VertexFormat::P3F_T2S_C4B_N3F,
				world, distanceFromCam, pBoneDataDes->asBufferHandle());
		}
		else
		{
			world.transpose();

			addMeshTobucket(mesh, renderMesh, render::shader::VertexFormat::P3F_T2S_C4B_N3F,
				world, distanceFromCam);
		}
	}

}


void World3D::addMeshTobucket(const model::MeshHeader& mesh, const model::XRenderMesh& renderMesh,
	render::shader::VertexFormat::Enum vertFmt, const Matrix44f& world, const float distanceFromCam, render::VertexBufferHandle boneData)
{
	render::CommandBucket<uint32_t>* pDepthBucket = pBucket_;

	render::VertexBufferHandleArr vertexBuffers = renderMesh.getVBBuffers();

	// we want to remove vertex buffer handles we don't need.
	// but can we do that on a perm mesh bases?
	// since with have diffrent materials 
	// so it's the requirement of the material as to what buffers it needs.
	const core::StrHash tech("unlit");

	engine::PermatationFlags permFlags = engine::PermatationFlags::VertStreams | engine::PermatationFlags::HwSkin;

	// now we render :D !
	for (size_t subIdx = 0; subIdx < mesh.numSubMeshes; subIdx++)
	{
		const model::SubMeshHeader* pSubMesh = mesh.subMeshHeads[subIdx];

		engine::Material* pMat = pSubMesh->pMat;
		engine::MaterialTech* pTech = engine::gEngEnv.pMaterialMan_->getTechForMaterial(
			pMat,
			tech,
			vertFmt,
			permFlags
		);

		if (!pTech) {
			continue;
		}

		const auto* pPerm = pTech->pPerm;
		const auto stateHandle = pPerm->stateHandle;
		auto* pVariableState = pTech->pVariableState;
		auto variableStateSize = pVariableState->getStateSize();

#if 1
		if (permFlags.IsSet(engine::PermatationFlags::HwSkin))
		{
			auto& buffers = pPerm->pShaderPerm->getBuffers();
			for (size_t i = 0; i < buffers.size(); i++)
			{
				auto& buf = buffers[i];

				if (buf.getName() == "BoneMatrices")
				{
					auto* pBuffers = pVariableState->getBuffers();

					pBuffers[i].buf = boneData;
					break;
				}
			}
		}
#endif

		uint32_t sortKey = static_cast<uint32_t>(distanceFromCam);

		// work out which cbuffers need updating.
		render::Commands::CopyConstantBufferData* pCBUpdate = nullptr;
		render::Commands::DrawIndexed* pDraw = nullptr;

		{
			int32_t numCbs = pVariableState->getNumCBs();
			auto* pCBHandles = pVariableState->getCBs();

			const auto& cbs = pTech->cbs;
			X_ASSERT(numCbs == static_cast<int32_t>(cbs.size()), "Size mismatch")(numCbs, cbs.size());

			for (int32_t i = 0; i < numCbs; i++)
			{
				auto& cb = *cbs[i];

				if (!cb.containsUpdateFreqs(render::shader::UpdateFreq::INSTANCE)) {
					continue;
				}

				// If we need to update the cbuffer for any reason we must provide the full cbuffer.
				size_t size = cb.getBindSize();
				char* pAuxData;

				if (pCBUpdate) {
					std::tie(pCBUpdate, pAuxData) = pDepthBucket->appendCommandGetAux<render::Commands::CopyConstantBufferData>(pCBUpdate, size);
				}
				else {
					std::tie(pCBUpdate, pAuxData) = pDepthBucket->addCommandGetAux<render::Commands::CopyConstantBufferData>(sortKey, size);
				}
				pCBUpdate->constantBuffer = pCBHandles[i];
				pCBUpdate->pData = pAuxData;
				pCBUpdate->size = static_cast<uint32_t>(size);

				// copy the cbuffer cpu that contains material param values.
				if (cb.containsUpdateFreqs(render::shader::UpdateFreq::MATERIAL)) {
					auto& cpuData = cb.getCpuData();
					std::memcpy(pAuxData, cpuData.data(), cpuData.size());
				}
				else {
					// this will be a instance from the shader perm, so unlikley to contain any useful data.
				}

				// now patch in any other params.
				for (int32_t paramIdx = 0; paramIdx < cb.getParamCount(); paramIdx++)
				{
					const auto& p = cb[paramIdx];
					const auto type = p.getType();
					const auto updateFreq = p.getUpdateRate();

					using namespace render;

					switch (updateFreq)
					{
						case shader::UpdateFreq::MATERIAL:
							continue;
						case shader::UpdateFreq::FRAME:
							// ask the cbuffer man to fill us? ;)
							pCBufMan_->setParamValue(type, (uint8_t*)&pAuxData[p.getBindPoint()]);
							continue;

						case shader::UpdateFreq::BATCH:
						case shader::UpdateFreq::SKINDATA:
						case shader::UpdateFreq::UNKNOWN:
							X_ASSERT_NOT_IMPLEMENTED();
							// ya fooking wut. NO!
							continue;

						case shader::UpdateFreq::INSTANCE:
							switch (type)
							{
								case shader::ParamType::PI_worldMatrix:
									std::memcpy(&pAuxData[p.getBindPoint()], &world, sizeof(world));
									break;
								case shader::ParamType::PI_objectToWorldMatrix:
									X_ASSERT_NOT_IMPLEMENTED();
									break;
								case shader::ParamType::PI_worldViewProjectionMatrix:
									X_ASSERT_NOT_IMPLEMENTED();
									break;
							}
							break;

					}
				}
			}

		}

		if (pCBUpdate) {
			pDraw = pDepthBucket->appendCommand<render::Commands::DrawIndexed>(pCBUpdate, variableStateSize);
		}
		else {
			pDraw = pDepthBucket->addCommand<render::Commands::DrawIndexed>(sortKey, variableStateSize);
		}

		pDraw->indexCount = pSubMesh->numIndexes;
		pDraw->startIndex = pSubMesh->startIndex;
		pDraw->baseVertex = pSubMesh->startVertex;
		pDraw->stateHandle = stateHandle;
		pDraw->resourceState = *pVariableState; // slice the sizes into command.
												// set the vertex handle to correct one.
		pDraw->indexBuffer = renderMesh.getIBBuffer();
		pDraw->vertexBuffers = vertexBuffers;

		if (variableStateSize)
		{
			char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDraw);
			std::memcpy(pAuxData, pVariableState->getDataStart(), pVariableState->getStateSize());
		}
	}
}

void World3D::addMeshTobucket(const model::MeshHeader& mesh, const model::XRenderMesh& renderMesh,
	render::shader::VertexFormat::Enum vertFmt, const Matrix44f& world, const float distanceFromCam)
{
	render::CommandBucket<uint32_t>* pDepthBucket = pBucket_;

	render::VertexBufferHandleArr vertexBuffers = renderMesh.getVBBuffers();

	const core::StrHash tech("unlit");

	engine::PermatationFlags permFlags = engine::PermatationFlags::VertStreams;

	{
		// zero them so render system don't prepare them.
		vertexBuffers[VertexStream::HWSKIN] = render::INVALID_BUF_HANLDE;
		vertexBuffers[VertexStream::INSTANCE] = render::INVALID_BUF_HANLDE;
	}

	// now we render :D !
	for (size_t subIdx = 0; subIdx < mesh.numSubMeshes; subIdx++)
	{
		const model::SubMeshHeader* pSubMesh = mesh.subMeshHeads[subIdx];

		engine::Material* pMat = pSubMesh->pMat;
		engine::MaterialTech* pTech = engine::gEngEnv.pMaterialMan_->getTechForMaterial(
			pMat, 
			tech, 
			vertFmt,
			permFlags
		);

		if (!pTech) {
			continue;
		}
		
		const auto* pPerm = pTech->pPerm;
		const auto stateHandle = pPerm->stateHandle;
		auto* pVariableState = pTech->pVariableState;
		auto variableStateSize = pVariableState->getStateSize();
	
		uint32_t sortKey = static_cast<uint32_t>(distanceFromCam);

		// work out which cbuffers need updating.
		render::Commands::CopyConstantBufferData* pCBUpdate = nullptr;
		render::Commands::DrawIndexed* pDraw = nullptr;

		{
			int32_t numCbs = pVariableState->getNumCBs();
			auto* pCBHandles = pVariableState->getCBs();

			const auto& cbs = pTech->cbs;
			X_ASSERT(numCbs == static_cast<int32_t>(cbs.size()), "Size mismatch")(numCbs, cbs.size());

			for (int32_t i = 0; i < numCbs; i++)
			{
				auto& cb = *cbs[i];

				// what are the cases we need to update a cbuffer?

				// can skip:
				// * it's per frame only.
				// * it's per material static only.
				// * it's per material static / dynamic only (these are updated at start of frame)
				// needs update:
				// * contains any per instance params.
				// * skin data?
				// * batch based? (what even is a batch? instanced? dunno.. what I intented it for)

				if (!cb.containsUpdateFreqs(render::shader::UpdateFreq::INSTANCE)) {
					continue;
				}

				// If we need to update the cbuffer for any reason we must provide the full cbuffer.
				size_t size = cb.getBindSize();
				char* pAuxData;

				if (pCBUpdate) {
					std::tie(pCBUpdate, pAuxData) = pDepthBucket->appendCommandGetAux<render::Commands::CopyConstantBufferData>(pCBUpdate, size);
				}
				else {
					std::tie(pCBUpdate, pAuxData) = pDepthBucket->addCommandGetAux<render::Commands::CopyConstantBufferData>(sortKey, size);
				}
				pCBUpdate->constantBuffer = pCBHandles[i];
				pCBUpdate->pData = pAuxData;
				pCBUpdate->size = static_cast<uint32_t>(size);

				// copy the cbuffer cpu that contains material param values.
				if (cb.containsUpdateFreqs(render::shader::UpdateFreq::MATERIAL)) {
					auto& cpuData = cb.getCpuData();
					std::memcpy(pAuxData, cpuData.data(), cpuData.size());
				}
				else {
					// this will be a instance from the shader perm, so unlikley to contain any useful data.
				}

				// now patch in any other params.
				for (int32_t paramIdx = 0; paramIdx < cb.getParamCount(); paramIdx++)
				{
					const auto& p = cb[paramIdx];
					const auto type = p.getType();
					const auto updateFreq = p.getUpdateRate();

					using namespace render;

					switch (updateFreq)
					{
						case shader::UpdateFreq::MATERIAL:
							continue;
						case shader::UpdateFreq::FRAME:
							// ask the cbuffer man to fill us? ;)
							pCBufMan_->setParamValue(type, (uint8_t*)&pAuxData[p.getBindPoint()]);
							continue;

						case shader::UpdateFreq::BATCH:
						case shader::UpdateFreq::SKINDATA:
						case shader::UpdateFreq::UNKNOWN:
							X_ASSERT_NOT_IMPLEMENTED();
							// ya fooking wut. NO!
							continue;

						case shader::UpdateFreq::INSTANCE:
							switch (type)
							{
								case shader::ParamType::PI_worldMatrix:
									std::memcpy(&pAuxData[p.getBindPoint()], &world, sizeof(world));
									break;
								case shader::ParamType::PI_objectToWorldMatrix:
									X_ASSERT_NOT_IMPLEMENTED();
									break;
								case shader::ParamType::PI_worldViewProjectionMatrix:
									X_ASSERT_NOT_IMPLEMENTED();
									break;
							}
							break;

					}
				}
			}

		}

		if (pCBUpdate) {
			pDraw = pDepthBucket->appendCommand<render::Commands::DrawIndexed>(pCBUpdate, variableStateSize);
		}
		else {
			pDraw = pDepthBucket->addCommand<render::Commands::DrawIndexed>(sortKey, variableStateSize);
		}

		pDraw->indexCount = pSubMesh->numIndexes;
		pDraw->startIndex = pSubMesh->startIndex;
		pDraw->baseVertex = pSubMesh->startVertex;
		pDraw->stateHandle = stateHandle;
		pDraw->resourceState = *pVariableState; // slice the sizes into command.
												// set the vertex handle to correct one.
		pDraw->indexBuffer = renderMesh.getIBBuffer();
		pDraw->vertexBuffers = vertexBuffers;

		if (variableStateSize)
		{
			// variable state data.
			char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pDraw);
			std::memcpy(pAuxData, pVariableState->getDataStart(), pVariableState->getStateSize());
		}
	}
}

// ---------------------------------------------------


void World3D::drawDebug(void)
{
	debugDraw_AreaBounds();
	debugDraw_Portals();
	debugDraw_PortalStacks();
	debugDraw_Lights();
	//	debugDraw_StaticModelCullVis();
	//	debugDraw_ModelBones();
	//	debugDraw_DrawDetachedCam();

}

void World3D::debugDraw_AreaBounds(void) const
{
	if (vars_.drawAreaBounds())
	{
		Color color = Col_Red;

		if (vars_.drawAreaBounds() == 1 || vars_.drawAreaBounds() == 3)
		{
			for (const auto& a : areas_)
			{
				if (isAreaVisible(a))
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

		if (vars_.drawAreaBounds() > 2)
		{
			color.a = 0.2f;

			// visible only
			if (vars_.drawAreaBounds() == 3)
			{
				for (const auto& a : areas_)
				{
					if (isAreaVisible(a))
					{
						pPrimContex_->drawAABB(a.pMesh->boundingBox, true, color);
					}
				}
			}
			else // all
			{
				for (const auto& a : areas_)
				{
					pPrimContex_->drawAABB(a.pMesh->boundingBox, true, color);
				}
			}
		}
	}
}

void World3D::debugDraw_Portals(void) const
{
	if (vars_.drawPortals() > 0 /* && !outsideWorld_ */)
	{
		// draw the portals.
		AreaArr::ConstIterator areaIt = areas_.begin();
		for (; areaIt != areas_.end(); ++areaIt)
		{
			if (!isAreaVisible(*areaIt)) {
				continue;
			}

			if (vars_.drawPortals() > 1)
			{
				pPrimContex_->setDepthTest(1);
			}

			Area::AreaPortalArr::ConstIterator apIt = areaIt->portals.begin();
			for (; apIt != areaIt->portals.end(); ++apIt)
			{
				const AreaPortal& portal = *apIt;

				if (isAreaVisible(areas_[portal.areaTo]))
				{
					pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
						portal.debugVerts.size(), Colorf(0.f, 1.f, 0.f, 0.35f));
				}
				else
				{
					pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
						portal.debugVerts.size(), Colorf(1.f, 0.f, 0.f, 0.3f));
				}
			}

			if (vars_.drawPortals() > 1)
			{
				pPrimContex_->setDepthTest(0);
			}
		}
	}
}


void World3D::debugDraw_PortalStacks(void) const
{
	if (vars_.drawPortalStacks())
	{
		// i wanna draw me the planes!
		// we have a clipped shape, described by a collection of planes.
		// how to i turn that into a visible shape.
		// in order to create the shape we need to clip the planes with each other.

		for (const auto& a : areas_)
		{
			if (!isAreaVisible(a)) {
				continue;
			}

			for (const auto& vp : a.visPortals)
			{
				const auto& portalPlanes = vp.planes;

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

void World3D::debugDraw_Lights(void) const
{
	pPrimContex_->setDepthTest(true);
	for (auto& light : lights_)
	{
		Sphere s(light.pos, 2.f);

		pPrimContex_->drawSphere(s, light.col, true);
	}
}



X_NAMESPACE_END