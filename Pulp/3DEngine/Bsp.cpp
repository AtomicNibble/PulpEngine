#include "stdafx.h"
#include "Bsp.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>

#include <Memory\MemCursor.h>

X_NAMESPACE_BEGIN(level)

namespace
{



}

Level::Level() :
	numAreas_(0),
	areaModels_(g_3dEngineArena)
{
	pFileData_ = nullptr;

	core::zero_object(fileHdr_);
}

Level::~Level()
{
	free();
}

void Level::free(void)
{
	if (pFileData_) {
		X_DELETE_ARRAY(pFileData_, g_3dEngineArena);
		pFileData_ = nullptr;
	}
}

bool Level::render(void)
{
	core::Array<AreaModel>::ConstIterator it = areaModels_.begin();
	for (; it != areaModels_.end(); ++it)
	{
		it->pRenderMesh->render();
	}

	return true;
}


bool Level::Load(const char* mapName)
{
	// does the other level file exsist?
	core::Path levelFile(mapName);
	levelFile.setExtension(level::LVL_FILE_EXTENSION);

	if (!gEnv->pFileSys->fileExists(levelFile.c_str())) {
		X_ERROR("Level", "could not find level file: \"%s\"",
			levelFile.c_str());
		return false;
	}

	X_LOG0("3DEngine", "Loading level: %s", mapName);
	loadStats_ = LoadStats();
	loadStats_.startTime = gEnv->pTimer->GetTimeReal();

	// dispatch a read request for the header.
	core::zero_object(fileHdr_);



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
