#include "stdafx.h"
#include "Level.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>
#include <IConsole.h>

#include <Memory\MemCursor.h>

#include <IRenderAux.h>

X_NAMESPACE_BEGIN(level)

AsyncLoadData::~AsyncLoadData()
{
}

// --------------------------------

int Level::s_var_drawAreaBounds_ = 0;


// --------------------------------

Level::Level() :
areaModels_(g_3dEngineArena),
stringTable_(g_3dEngineArena)
{
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

	ADD_CVAR_REF("lvl_drawAreaBounds", s_var_drawAreaBounds_, 0, 0, 1, core::VarFlag::SYSTEM,
		"Draws bounding box around each level area");


	return true;
}

void Level::ShutDown(void)
{


}

void Level::update(void)
{
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

	// clear render mesh.
	core::Array<AreaModel>::ConstIterator it = areaModels_.begin();
	for (; it != areaModels_.end(); ++it)
	{
		it->pRenderMesh->release();
	}

	areaModels_.free();

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

	core::Array<AreaModel>::ConstIterator it = areaModels_.begin();
	for (; it != areaModels_.end(); ++it)
	{
		it->pRenderMesh->render();
	}

	if (s_var_drawAreaBounds_)
	{
		render::IRenderAux* pAux = gEnv->pRender->GetIRenderAuxGeo();

		pAux->setRenderFlags(render::AuxGeom_Defaults::Def3DRenderflags);

		it = areaModels_.begin();
		for (; it != areaModels_.end(); ++it)
		{
			Vec3f pos = Vec3f::zero();
			pAux->drawAABB(it->pMesh->boundingBox,pos, false, Col_Red);
		}
	}

	return true;
}


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

	if (fileHdr_.datasize <= 0)
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

bool Level::ProcessData(uint32_t bytesRead)
{
	if (bytesRead != fileHdr_.totalDataSize) {
		X_ERROR("Level", "failed to read level file. read: %i of %i",
			bytesRead, fileHdr_.totalDataSize);
		return false;
	}

	// read string table.
	core::XFileBuf file(pFileData_, pFileData_ + fileHdr_.stringDataSize);

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

	core::StackString<64> meshName;
	core::MemCursor<uint8_t> cursor(pFileData_ + fileHdr_.stringDataSize, fileHdr_.datasize);
	uint32_t x, numSub;

	areaModels_.reserve(fileHdr_.numAreas);
	for (uint32_t i = 0; i < fileHdr_.numAreas; i++)
	{
		model::MeshHeader* pMesh = cursor.getSeekPtr<model::MeshHeader>();
		numSub = pMesh->numSubMeshes;

		X_ASSERT(numSub > 0, "a areamodel can't have zero meshes")(numSub);

		// set meshHeads verts and faces.
		pMesh->subMeshHeads = cursor.postSeekPtr<model::SubMeshHeader>(numSub);

		// verts
		pMesh->streams[VertexStream::VERT] = cursor.postSeekPtr<uint8_t>(pMesh->numVerts * 
			((sizeof(Vec2f)*2) + sizeof(Vec3f)));
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
			uint32_t matID = reinterpret_cast<uint32_t>(pSubMesh->materialName.as<uint32_t>());
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

		AreaModel area;
		area.pMesh = pMesh;
		area.pRenderMesh = gEnv->pRender->createRenderMesh(pMesh,
			shader::VertexFormat::P3F_T4F_C4B_N3F, meshName.c_str());
		area.pRenderMesh->uploadToGpu();

		areaModels_.append(area);
	}

	if (!cursor.isEof())
	{
		X_WARNING("Level", "potential read error, cursor is not at end. bytes left: %i",
			cursor.numBytesRemaning());
	}

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

#if 0
bool Level::LoadFromFile(const char* filename)
{
	X_ASSERT_NOT_NULL(filename);

	core::fileModeFlags mode;
	FileHeader hdr;
	core::Path path;
	bool LumpsLoaded;

	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::RANDOM_ACCESS); // the format allows for each lump to be in any order currently.

	core::zero_object(hdr);

	path.append(filename);
	path.setExtension(level::LVL_FILE_EXTENSION);

	LumpsLoaded = false;

	core::XFileMemScoped file;
	if (file.openFile(path.c_str(), mode))
	{
		file->readObj(hdr);

		// is this header valid m'lady?
		if (!hdr.isValid())
		{
			// this header is not valid :Z
			if (hdr.fourCC == LVL_FOURCC_INVALID)
			{
				X_ERROR("Bsp", "%s file is corrupt, please re-compile.", path.fileName());
			}
			else
			{
				X_ERROR("Bsp", "%s is not a valid bsp", path.fileName());
			}

			return false;
		}

		if (hdr.version != LVL_VERSION)
		{
			X_ERROR("Bsp", "%s has a invalid version. provided: %i required: %i",
				path.fileName(), hdr.version, LVL_VERSION);
			return false;
		}

		if (hdr.datasize <= 0)
		{
			X_ERROR("Bsp", "level file is empty");
			return false;
		}

		// require atleast one area.
		if(hdr.numAreas < 1)
		{
			X_ERROR("Bsp", "Level file has no areas");
			return false;
		}

		// copy some stuff.
		numAreas_ = hdr.numAreas;


		pFileData_ = X_NEW_ARRAY(uint8_t, hdr.datasize, g_3dEngineArena, "LevelBuffer");
		
		uint32_t bytesRead = file->read(pFileData_, hdr.datasize);
		if (bytesRead != hdr.datasize)
		{
			X_ERROR("Bsp", "failed to read level file. read: %i of %i",
				bytesRead, hdr.datasize);

			X_DELETE_ARRAY(pFileData_,g_3dEngineArena);
			pFileData_ = nullptr;
			return false;
		}

		core::StackString<64> meshName;
		core::MemCursor<uint8_t> cursor(pFileData_, hdr.datasize);
		uint32_t x, numSub;

		areaModels_.reserve(hdr.numAreas);
		for (uint32_t i = 0; i < hdr.numAreas; i++)
		{
			model::MeshHeader* pMesh = cursor.getSeekPtr<model::MeshHeader>();		
			numSub = pMesh->numSubMeshes;

			X_ASSERT(numSub > 0, "a areamodel can't have zero meshes")(numSub);

			// set meshHeads verts and faces.
			pMesh->subMeshHeads = cursor.postSeekPtr<model::SubMeshHeader>(numSub);
			
			// verts
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->verts = cursor.postSeekPtr<level::Vertex>(pSubMesh->numVerts);;
			}

			// indexs
			for (x = 0; x < numSub; x++)
			{
				model::SubMeshHeader* pSubMesh = pMesh->subMeshHeads[x];
				pSubMesh->indexes = cursor.postSeekPtr<model::Index>(pSubMesh->numIndexes);
			}

			// set the mesh head pointers.
			pMesh->streams[VertexStream::VERT] = pMesh->subMeshHeads[0]->verts;
			pMesh->indexes = pMesh->subMeshHeads[0]->indexes;

			meshName.clear();
			meshName.appendFmt("$area_mesh%i", i);

			AreaModel area;
			area.pMesh = pMesh;
			area.pRenderMesh = gEnv->pRender->createRenderMesh(pMesh, 
				shader::VertexFormat::P3F_T4F_C4B_N3F, meshName.c_str());
			area.pRenderMesh->uploadToGpu();

			areaModels_.append(area);
		}


		if(!cursor.isEof())
		{
			X_WARNING("Bsp", "potential read error, cursor is not at end");
		}


		file.close(); // not needed but makes my nipples more perky.
		return true;
	}

	return false;
}
#endif

X_NAMESPACE_END
