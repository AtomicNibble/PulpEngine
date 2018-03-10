#include "stdafx.h"
#include "Level.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>

#include <Memory\MemCursor.h>
#include <Math\XWinding.h>
#include <Threading\JobSystem2.h>

#include "Model\ModelManager.h"
#include "Material\MaterialManager.h"


X_NAMESPACE_BEGIN(level)


void Level::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = pRequest->getType();

	if (requestType == core::IoRequest::OPEN)
	{
		if (!pFile) {
			X_ERROR("Level", "Failed to open level file");
			return;
		}
		
		core::IoRequestRead read;
		read.callback.Bind<Level, &Level::IoRequestCallback>(this);
		read.pFile = pFile;
		read.dataSize = sizeof(fileHdr_);
		read.offset = 0;
		read.pBuf = &fileHdr_;

		fileSys.AddIoRequestToQue(read);
	}
	else if (requestType == core::IoRequest::READ)
	{
		if (!bytesTransferred) {
			X_ERROR("Level", "Failed to read level data");
			return;
		}
		core::V2::Job* pJob = nullptr;

		if (!headerLoaded_) {
			pJob = pJobSys_->CreateMemberJob<Level>(this, &Level::ProcessHeader_job, pFile JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		}
		else {
			pJob = pJobSys_->CreateMemberJob<Level>(this, &Level::ProcessData_job, pFile JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		}
		
		pJobSys_->Run(pJob);
	}
}



void Level::ProcessHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	core::XFileAsync* pFile = static_cast<core::XFileAsync*>(pData);

	// check if the header is correct.
	if (ProcessHeader()) 
	{
		headerLoaded_ = true;

		// allocate buffer for the file data.
		uint32_t dataSize = fileHdr_.totalDataSize;

		pFileData_ = X_NEW_ARRAY_ALIGNED(uint8_t, dataSize, g_3dEngineArena, "LevelBuffer", 16);


		core::IoRequestRead read;
		read.callback.Bind<Level, &Level::IoRequestCallback>(this);
		read.dataSize = dataSize;
		read.offset = sizeof(fileHdr_);
		read.pBuf = pFileData_;
		read.pFile = pFile;

		pFileSys_->AddIoRequestToQue(read);
	}
}

void Level::ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	X_ASSERT(headerLoaded_, "Header must be loaded in order to process data")(headerLoaded_);
	X_ASSERT_NOT_NULL(pFileData_);

	core::XFileAsync* pFile = static_cast<core::XFileAsync*>(pData);

	ProcessData();


	core::IoRequestClose req;
	req.pFile = pFile;
	pFileSys_->AddIoRequestToQue(req);
}


bool Level::ProcessHeader(void)
{
	// is this header valid m'lady?
	if (!fileHdr_.isValid())
	{
		// this header is not valid :Z
		if (fileHdr_.fourCC == LVL_FOURCC_INVALID)
		{
			X_ERROR("Level", "%s file is corrupt, please re-compile.", path_.fileName());
			return false;
		}

		X_ERROR("Level", "%s is not a valid level", path_.fileName());
		return false;
	}

	if (fileHdr_.version != LVL_VERSION)
	{
		X_ERROR("Level", "%s has a invalid version. provided: %i required: %i",
			path_.fileName(), fileHdr_.version, LVL_VERSION);
		return false;
	}

	if (fileHdr_.totalDataSize <= 0)
	{
		X_ERROR("Level", "level file is empty");
		return false;
	}

	// require atleast one area.
	if (fileHdr_.numAreas < 1)
	{
		X_ERROR("Level", "Level file has no areas");
		return false;
	}

	return true;
}


bool ProcessIAP(core::XFileFixedBuf& file, Area& area, int32_t areaTo)
{
	AreaPortal& p = area.portals.AddOne();
	p.pWinding = X_NEW(XWinding, g_3dEngineArena, "AreaPortalWinding");
	p.areaTo = areaTo;
	if (!p.pWinding->SLoad(&file)) {
		X_ERROR("Level", "Failed to load iap winding");
		X_DELETE(p.pWinding, g_3dEngineArena);
		return false;
	}

	file.readObj(p.plane);

	// read debugVerts.
	uint32_t numVerts;
	if (!file.readObj(numVerts)) {
		X_ERROR("Level", "Failed to load iap dv's");
		return false;
	}

	p.debugVerts.setGranularity(numVerts);
	p.debugVerts.resize(numVerts);

	// read them.
	if (!file.readObj(p.debugVerts.ptr(), numVerts)) {
		X_ERROR("Level", "Failed to read iap verts");
		return false;
	}

	return true;
}

bool Level::ProcessData(void)
{
	X_ASSERT_ALIGNMENT(pFileData_, 16, 0);

	// read string table.
	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::STRING_TABLE);

		if (!stringTable_.SLoad(&file))
		{
			X_ERROR("Level", "Failed to load string table.");
			return false;
		}

		if (!file.isEof())
		{
			X_ERROR("Level", "Failed to fully parse sting table.");
			return false;
		}
	}

	areas_.resize(fileHdr_.numAreas);

	// area data.
	{
		core::StackString<assetDb::ASSET_NAME_MAX_LENGTH> meshName;
		core::MemCursor cursor(pFileData_ + fileHdr_.nodes[FileNodes::AREA_MODELS].offset,
			fileHdr_.nodes[FileNodes::AREA_MODELS].size);
		uint32_t x, numSub;

		for (int32_t i = 0; i < fileHdr_.numAreas; i++)
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
				pSubMesh->indexes = cursor.postSeekPtr<model::Index>(pSubMesh->numFaces * 3);
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
				pSubMesh->materialName = stringTable_.getString(matID);
			}

			// load materials
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];

				pSubMesh->pMat = engine::gEngEnv.pMaterialMan_->loadMaterial(pSubMesh->materialName);
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
			X_WARNING("Level", "potential read error, cursor is not at end. bytes left: %i",
				cursor.numBytesRemaning());
		}
	}

	// area Portals
	if (fileHdr_.flags.IsSet(LevelFileFlags::INTER_AREA_INFO))
	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_PORTALS);

		// 2 ints for the area numbers followed by a winding.
		uint32_t i, numIaps = fileHdr_.numinterAreaPortals;

		for (i = 0; i < numIaps; i++)
		{
			int32_t a1, a2;

			file.readObj(a1);
			file.readObj(a2);

			// check the area numbers are valid.
			if (a1 < 0 || a1 > fileHdr_.numAreas) {
				X_ERROR("Level", "Invalid interarea area: %i valid-range(0,%i)",
					a1, fileHdr_.numAreas);
				return false;
			}
			if (a2 < 0 || a2 > fileHdr_.numAreas) {
				X_ERROR("Level", "Invalid interarea area: %i valid-range(0,%i)",
					a2, fileHdr_.numAreas);
				return false;
			}

			// add to areas
			Area& area1 = areas_[a1];
			Area& area2 = areas_[a2];

			++area1.maxVisPortals;
			++area2.maxVisPortals;

			ProcessIAP(file, area1, a2);
			ProcessIAP(file, area2, a1);
#if 0
			AreaPortal& p1 = area1.portals.AddOne();
			AreaPortal& p2 = area2.portals.AddOne();

			// get p1's plane from the winding.
			pWinding->getPlane(p1.plane);
			p1.pWinding = pWinding;
			p1.areaTo = a2;

			p2.pWinding = pWinding->ReverseWinding();
			// p2's plane is made from the reverse winding.
			p2.pWinding->getPlane(p2.plane);
			p2.areaTo = a1;
#endif
		}



		for (auto& a : areas_)
		{
			a.maxVisPortals = core::Max(1, a.maxVisPortals);
			a.visPortals.resize(a.maxVisPortals);
		}

		if (!file.isEof()) {
			X_WARNING("Level", "potential read error, "
				"failed to process all bytes for node %i. bytes left: %i",
				FileNodes::AREA_PORTALS, file.remainingBytes());
		}
	}
	else
	{
		X_WARNING("Level", "Level has no inter area portals.");
	}

#if 0

	if (fileHdr_.flags.IsSet(LevelFileFlags::AREA_ENT_REF_LISTS))
	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_ENT_REFS);

		AreaRefInfo& entRefs = entRefs_;
		entRefs.areaRefHdrs.resize(fileHdr_.numAreas);

		file.readObj(entRefs.areaRefHdrs.ptr(), entRefs.areaRefHdrs.size());

		size_t numEntRefs = fileHdr_.numEntRefs;
		size_t numMultiAreaEntRefs = fileHdr_.numMultiAreaEntRefs;


		// load into single buffer.
		entRefs.areaRefs.resize(numEntRefs);
		file.readObj(entRefs.areaRefs.ptr(), entRefs.areaRefs.size());

		// load multi area ref list headers.
		file.readObj(entRefs.areaMultiRefHdrs.data(), entRefs.areaMultiRefHdrs.size());

		// load the multi area ref lists data.
		entRefs.areaMultiRefs.resize(numMultiAreaEntRefs);
		file.readObj(entRefs.areaMultiRefs.ptr(), entRefs.areaMultiRefs.size());
	}

	if (fileHdr_.flags.IsSet(LevelFileFlags::AREA_MODEL_REF_LISTS))
	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_MODEL_REFS);

		AreaRefInfo& modelRefs = modelRefs_;
		modelRefs.areaRefHdrs.resize(fileHdr_.numAreas);

		file.readObj(modelRefs.areaRefHdrs.ptr(), modelRefs.areaRefHdrs.size());

		size_t numModelRefs = fileHdr_.numModelRefs;
		size_t numMultiAreaModelRefs = fileHdr_.numMultiAreaModelRefs;


		// load into single buffer.
		modelRefs.areaRefs.resize(numModelRefs);
		file.readObj(modelRefs.areaRefs.ptr(), modelRefs.areaRefs.size());

		// load multi area ref list headers.
		file.readObj(modelRefs.areaMultiRefHdrs.data(), modelRefs.areaMultiRefHdrs.size());

		// load the multi area ref lists data.
		modelRefs.areaMultiRefs.resize(numMultiAreaModelRefs);
		file.readObj(modelRefs.areaMultiRefs.ptr(), modelRefs.areaMultiRefs.size());

#if 0
		for (size_t b = 0; b < fileHdr_.numAreas; b++)
		{
			const FileAreaRefHdr& areaModelsHdr = modelRefs.areaRefHdrs[b];
			X_LOG0("modelRefs", "%i start: %i num: %i", b, areaModelsHdr.startIndex, areaModelsHdr.num);
		}
#endif
	}
#endif

	if (fileHdr_.flags.IsSet(LevelFileFlags::COLLISION))
	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_COLLISION);

		// add all the area bounds
		for (const auto& a : areas_)
		{
			pScene_->addRegion(a.getBounds());
		}

		physics::IPhysics::DataArr data(g_3dEngineArena);

		for (auto& a : areas_)
		{
			AreaCollisionHdr colHdr;
			file.readObj(colHdr);

			Transformf trans;
			trans.pos = colHdr.trans;

			for (uint8_t i=0; i<colHdr.numGroups; i++)
			{
				AreaCollisionGroupHdr groupHdr;
				file.readObj(groupHdr);

				static_assert(CollisionDataType::ENUM_COUNT == 4, "Enum count changed? this code may need updating");

				AreaCollisionDataHdr dataHdr;

				auto actor = gEnv->pPhysics->createStaticActor(trans, &a);

				for (uint8_t t = 0; t < groupHdr.numTypes[CollisionDataType::TriMesh]; t++)
				{
					file.readObj(dataHdr);
					data.resize(dataHdr.dataSize);
					file.read(data.data(), data.size());

					auto triMeshHandle = gEnv->pPhysics->createTriangleMesh(data);
					gEnv->pPhysics->addTriMesh(actor, triMeshHandle);
				}

				for (uint8_t t = 0; t < groupHdr.numTypes[CollisionDataType::ConvexMesh]; t++)
				{
					file.readObj(dataHdr);
					data.resize(dataHdr.dataSize);
					file.read(data.data(), data.size());

					auto convexMeshHandle = gEnv->pPhysics->createConvexMesh(data);
					gEnv->pPhysics->addConvexMesh(actor, convexMeshHandle);
				}

				for (uint8_t t = 0; t < groupHdr.numTypes[CollisionDataType::HeightField]; t++)
				{
					file.readObj(dataHdr);
					data.resize(dataHdr.dataSize);
					file.read(data.data(), data.size());

					auto hfHandle = gEnv->pPhysics->createHieghtField(data);
					gEnv->pPhysics->addHieghtField(actor, hfHandle);
				}

				AABB aabb;
				for (uint8_t t = 0; t < groupHdr.numTypes[CollisionDataType::Aabb]; t++)
				{
					file.readObj(aabb);

					// the AABB is world space right.
					// but the area actor is in area space.
					const Vec3f localPose = aabb.center() - trans.pos;

					gEnv->pPhysics->addBox(actor, aabb, localPose);
				}

				pScene_->addActorToScene(actor);

				a.physicsActor = actor;
			}
		}

	}

	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::STATIC_MODELS);

		staticModels_.resize(fileHdr_.numStaticModels);
		if (staticModels_.isNotEmpty())
		{
			// we read each item one by one since we are readong from a FileBuf.
			size_t i;
			for (i = 0; i < staticModels_.size(); i++)
			{
				StaticModel& sm = staticModels_[i];
				FileStaticModel fsm;

				file.readObj(fsm);

				// copy over the info.
		//		sm.pos = fsm.pos;
		//		sm.angle = fsm.angle;
				sm.modelNameIdx = fsm.modelNameIdx;
				sm.boundingBox = fsm.boundingBox;
				sm.boundingSphere = Sphere(fsm.boundingBox); // create sphere from AABB.
				// models need to be loaded at some point.

				const char* modelName = stringTable_.getString(sm.modelNameIdx);
				model::XModel* pModel = engine::gEngEnv.pModelMan_->loadModel(modelName);
				
		//		X_LOG0("SM", "%i name: %s pos: (%g,%g,%g,)", i,  modelName,
		//			sm.pos[0], sm.pos[1], sm.pos[2]);

				sm.pModel = pModel;
			}
		}
	}

	// nodes
	if (fileHdr_.flags.IsSet(LevelFileFlags::BSP_TREE))
	{
		core::XFileFixedBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::BSP_TREE);

		areaNodes_.resize(fileHdr_.numNodes);

		// we read: nodeNumber, plane, childid's
		int32_t i;

		for (i = 0; i < fileHdr_.numNodes; i++)
		{
			AreaNode& node = areaNodes_[i];

			file.readObj(node.plane);
			file.readObj(node.children);
		}

		if (!file.isEof()) {
			X_WARNING("Level", "potential read error, "
				"failed to process all bytes for node %i. bytes left: %" PRIuS,
				FileNodes::BSP_TREE, file.remainingBytes());
		}
	}
	else
	{
		X_WARNING("Level", "Level has no area tree.");
	}


	// 
	CommonChildrenArea_r(&areaNodes_[0]);

	// stats.
	loadStats_.elapse = pTimer_->GetTimeNowNoScale() - loadStats_.startTime;

	// lock and load.
	loaded_ = true;

	X_LOG0("Level", "%s loaded in %gms",
		path_.fileName(),
		loadStats_.elapse.GetMilliSeconds());

#if 1
	Transformf trans;

	for (size_t i = 0; i < 30; i++)
	{
		trans.pos.x = gEnv->xorShift.randRange(-200.f, 200.f);
		trans.pos.y = gEnv->xorShift.randRange(-200.f, 200.f);
		trans.pos.z = gEnv->xorShift.randRange(20.f, 100.f);

		float size = gEnv->xorShift.randRange(5.f, 20.f);

		auto box = gEnv->pPhysics->createBox(trans, AABB(Vec3f::zero(), size), 0.5f);
		pScene_->addActorToScene(box);
	}

	for (size_t i = 0; i < 20; i++)
	{
		trans.pos.x = -248;
		trans.pos.y = 180;
		trans.pos.z = 80;

		trans.pos.x += gEnv->xorShift.randRange(-20.f, 20.f);
		trans.pos.y += gEnv->xorShift.randRange(-20.f, 20.f);


		float size = gEnv->xorShift.randRange(1.f, 10.f);

		auto box = gEnv->pPhysics->createBox(trans, AABB(Vec3f::zero(), size), 0.5f);
		pScene_->addActorToScene(box);
	}

	trans.pos.y = 120;

	for (size_t i = 0; i < 10; i++)
	{
		float size = gEnv->xorShift.randRange(1.f, 10.f);

		trans.pos.y = 120;
		trans.pos.x += gEnv->xorShift.randRange(-20.f, 20.f);
		trans.pos.y += gEnv->xorShift.randRange(-20.f, 20.f);

		auto box = gEnv->pPhysics->createSphere(trans, size, 0.6f);
		pScene_->addActorToScene(box);
	}
#endif

	gEnv->pPhysics->addSceneToSim(pScene_);

	return true;
}

int32_t Level::CommonChildrenArea_r(AreaNode* pAreaNode)
{
	int32_t	nums[2];

	for (int i = 0; i < 2; i++)
	{
		if (pAreaNode->children[i] <= 0) {
			nums[i] = -1 - pAreaNode->children[i];
		}
		else {
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
	if (nums[0] == nums[1]) {
		common = nums[0];
	}
	else {
		common = AreaNode::CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	pAreaNode->commonChildrenArea = common;

	return common;
}


X_NAMESPACE_END