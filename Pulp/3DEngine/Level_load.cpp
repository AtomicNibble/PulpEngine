#include "stdafx.h"
#include "Level.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>

#include <Memory\MemCursor.h>

#include <Math\XWinding.h>

X_NAMESPACE_BEGIN(level)



bool Level::Load(const char* mapName)
{
	// does the other level file exsist?
	path_.set(mapName);
	path_.setExtension(level::LVL_FILE_EXTENSION);

	if (!gEnv->pFileSys->fileExists(path_.c_str())) {
		X_ERROR("Level", "could not find level file: \"%s\"",
			path_.c_str());
		return false;
	}

	// free it :)
	free();

	X_LOG0("Level", "Loading level: %s", mapName);
	loadStats_ = LoadStats();
	loadStats_.startTime = pTimer_->GetTimeReal();

	// clear it.
	core::zero_object(fileHdr_);

	core::fileModeFlags mode = core::fileMode::READ;
	core::XFileAsync* pFile = pFileSys_->openFileAsync(path_.c_str(), mode);
	if (!pFile) {
		return false;
	}

	core::XFileAsyncOperation HeaderOp = pFile->readAsync(&fileHdr_, sizeof(fileHdr_), 0);

	pAsyncLoadData_ = X_NEW(AsyncLoadData, g_3dEngineArena, "LevelLoadHandles")(pFile, HeaderOp);
	pAsyncLoadData_->waitingForIo_ = true;
	return true;
}

bool Level::ProcessHeader(uint32_t bytesRead)
{
	if (bytesRead != sizeof(fileHdr_)) {
		X_ERROR("Level", "failed to read header file: %s", path_.fileName());

		return false;
	}

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

	// allocate the file data.
	pFileData_ = X_NEW_ARRAY(uint8_t, fileHdr_.totalDataSize, g_3dEngineArena, "LevelBuffer");

	pAsyncLoadData_->AsyncOp_ = pAsyncLoadData_->pFile_->readAsync(pFileData_,
		fileHdr_.totalDataSize, sizeof(fileHdr_));

	pAsyncLoadData_->waitingForIo_ = true;
	return true;
}


bool ProcessIAP(core::XFileBuf& file, Area& area, int32_t areaTo)
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

	p.debugVerts.resize(numVerts);

	// read them.
	if (!file.readObj(p.debugVerts.ptr(), numVerts)) {

		return false;
	}

	return true;
}

bool Level::ProcessData(uint32_t bytesRead)
{
	if (bytesRead != fileHdr_.totalDataSize) {
		X_ERROR("Level", "failed to read level file. read: %i of %i",
			bytesRead, fileHdr_.totalDataSize);
		return false;
	}

	// read string table.
	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::STRING_TABLE);

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
		core::StackString<64> meshName;
		core::MemCursor<uint8_t> cursor(pFileData_ + fileHdr_.nodes[FileNodes::AREAS].offset,
			fileHdr_.nodes[FileNodes::AREAS].size);
		uint32_t x, numSub;

		for (int32_t i = 0; i < fileHdr_.numAreas; i++)
		{
			model::MeshHeader* pMesh = cursor.getSeekPtr<model::MeshHeader>();
			numSub = pMesh->numSubMeshes;

			X_ASSERT(numSub > 0, "a areamodel can't have zero meshes")(numSub);

			// set meshHeads verts and faces.
			pMesh->subMeshHeads = cursor.postSeekPtr<model::SubMeshHeader>(numSub);

			// verts
			pMesh->streams[VertexStream::VERT] = cursor.postSeekPtr<uint8_t>(pMesh->numVerts *
				((sizeof(Vec2f)* 2) + sizeof(Vec3f)));
			pMesh->streams[VertexStream::COLOR] = cursor.postSeekPtr<Color8u>(pMesh->numVerts);
			pMesh->streams[VertexStream::NORMALS] = cursor.postSeekPtr<Vec3f>(pMesh->numVerts);

			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->streams[VertexStream::VERT] += pMesh->streams[VertexStream::VERT];
				pSubMesh->streams[VertexStream::COLOR] += pMesh->streams[VertexStream::COLOR];
				pSubMesh->streams[VertexStream::NORMALS] += pMesh->streams[VertexStream::NORMALS];
			}

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
				pSubMesh->materialName = stringTable_.getString(matID);
			}

			// load materials
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];

				pSubMesh->pMat = pMaterialManager_->loadMaterial(pSubMesh->materialName);
			}

			// set the mesh head pointers.
			pMesh->indexes = pMesh->subMeshHeads[0]->indexes;

			meshName.clear();
			meshName.appendFmt("$area_mesh%i", i);

			// update area info.
			Area& area = areas_[i];
			area.areaNum = i;
			area.pMesh = pMesh;
			area.pRenderMesh = gEnv->pRender->createRenderMesh(pMesh,
				shader::VertexFormat::P3F_T4F_C4B_N3F, meshName.c_str());

			// upload to gpu now.
			area.pRenderMesh->uploadToGpu();
		}

		if (!cursor.isEof()) {
			X_WARNING("Level", "potential read error, cursor is not at end. bytes left: %i",
				cursor.numBytesRemaning());
		}
	}

	// area Portals
	if (fileHdr_.flags.IsSet(LevelFileFlags::INTER_AREA_INFO))
	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_PORTALS);

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

	
	if (fileHdr_.flags.IsSet(LevelFileFlags::AREA_ENT_REF_LISTS))
	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_ENT_REFS);

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
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::AREA_MODEL_REFS);

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

		for (size_t b = 0; b < fileHdr_.numAreas; b++)
		{
			const FileAreaRefHdr& areaModelsHdr = modelRefs.areaRefHdrs[b];
			X_LOG0("modelRefs", "%i start: %i num: %i", b, areaModelsHdr.startIndex, areaModelsHdr.num);
		}
	}

	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::STATIC_MODELS);

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
				sm.pos = fsm.pos;
				sm.angle = fsm.angle;
				sm.modelNameIdx = fsm.modelNameIdx;
				// models need to be loaded at some point.

				const char* modelName = stringTable_.getString(sm.modelNameIdx);
				model::IModel* pModel = getModelManager()->loadModel(modelName);
				
				X_LOG0("SM", "%i name: %s pos: (%g,%g,%g,)", i,  modelName,
					sm.pos[0], sm.pos[1], sm.pos[2]);

				sm.pModel = pModel;
			}
		}
	}

	// nodes
	if (fileHdr_.flags.IsSet(LevelFileFlags::BSP_TREE))
	{
		core::XFileBuf file = fileHdr_.FileBufForNode(pFileData_, FileNodes::BSP_TREE);

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
				"failed to process all bytes for node %i. bytes left: %i",
				FileNodes::BSP_TREE, file.remainingBytes());
		}
	}
	else
	{
		X_WARNING("Level", "Level has no area tree.");
	}


	// 
	CommonChildrenArea_r(&areaNodes_[0]);


	// clean up.
	pFileSys_->closeFileAsync(pAsyncLoadData_->pFile_);

	X_DELETE_AND_NULL(pAsyncLoadData_, g_3dEngineArena);

	// stats.
	loadStats_.elapse = pTimer_->GetTimeReal() - loadStats_.startTime;

	// safe to render?
	canRender_ = true;

	X_LOG0("Level", "%s loaded in %gms",
		path_.fileName(),
		loadStats_.elapse.GetMilliSeconds());

	return true;
}


X_NAMESPACE_END