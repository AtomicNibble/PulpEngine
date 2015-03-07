#include "stdafx.h"
#include "Bsp.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>

#include <Memory\MemCursor.h>

X_NAMESPACE_BEGIN(bsp)

namespace
{
	template<typename T>
	bool LoadLump(core::XFileMemScoped& file, const FileLump& lump, core::Array<T>& vec)
	{
		uint32_t num = lump.size / sizeof(T);

		// check lump size is valid.
		if ((num * sizeof(T)) != lump.size)
			return false;

		// a lump must have some sort of offset.
		if (lump.offset == 0)
			return false;

		vec.resize(num);

		// seek if needed.
		file->seek(lump.offset, core::SeekMode::SET);
		return file->read(vec.ptr(), lump.size) == lump.size;
	}

	template<typename T>
	bool IsBelowLimit(const FileHeader& hdr, LumpType::Enum type, uint32_t max)
	{
		uint32_t num = 0; // hdr.lumps[type].size / sizeof(T);

		if (num > max) 
		{
			X_ERROR("Bsp", "too many %s: %i max: %i",
				LumpType::ToString(type), num, max);
			return false;
		}
		return true;
	}

}

Bsp::Bsp() :
	data_(g_3dEngineArena),
	areaModels_(g_3dEngineArena)
{
	pFileData_ = nullptr;
}

Bsp::~Bsp()
{
	free();
}

void Bsp::free(void)
{
	if (pFileData_) {
		X_DELETE_ARRAY(pFileData_, g_3dEngineArena);
		pFileData_ = nullptr;
	}
}

bool Bsp::render(void)
{
	core::Array<AreaModel>::ConstIterator it = areaModels_.begin();
	for (; it != areaModels_.end(); ++it)
	{
		it->pRenderMesh->render();
	}

	return true;
}


bool Bsp::LoadFromFile(const char* filename)
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
	path.setExtension(bsp::BSP_FILE_EXTENSION);

	LumpsLoaded = false;

	core::XFileMemScoped file;
	if (file.openFile(path.c_str(), mode))
	{
		file->readObj(hdr);

		// is this header valid m'lady?
		if (!hdr.isValid())
		{
			// this header is not valid :Z
			if (hdr.fourCC == BSP_FOURCC_INVALID)
			{
				X_ERROR("Bsp", "%s file is corrupt, please re-compile.", path.fileName());
			}
			else
			{
				X_ERROR("Bsp", "%s is not a valid bsp", path.fileName());
			}

			return false;
		}

		if (hdr.version != BSP_VERSION)
		{
			X_ERROR("Bsp", "%s has a invalid version. provided: %i required: %i",
				path.fileName(), hdr.version, BSP_VERSION);
			return false;
		}

		if (hdr.datasize <= 0)
		{
			X_ERROR("Bsp", "level file is empty");
			return false;
		}


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

		areaModels_.reserve(hdr.numAreaModels);
		for (uint32_t i = 0; i < hdr.numAreaModels; i++)
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
				pSubMesh->verts = cursor.postSeekPtr<bsp::Vertex>(pSubMesh->numVerts);;
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

		// make some render meshes.

		

#if 0

		//	areaModels_.resize(hdr.numAreaModels);
		for (uint32_t i = 0; i < hdr.numAreaModels; i++)
		{
			//	file->readObj(areaModels_[i]);
		}

		// do some limit checking.
		// no pesky bsp's !
		bool belowLimits = true;
		belowLimits &= IsBelowLimit<Vertex>(hdr, LumpType::Materials, MAP_MAX_MATERIALs);
		belowLimits &= IsBelowLimit<Vertex>(hdr, LumpType::Planes, MAP_MAX_PLANES);
		belowLimits &= IsBelowLimit<Vertex>(hdr, LumpType::Verts, MAP_MAX_VERTS);
		belowLimits &= IsBelowLimit<Index>(hdr, LumpType::Indexes, MAP_MAX_INDEXES);
		belowLimits &= IsBelowLimit<Brush>(hdr, LumpType::Brushes, MAP_MAX_BRUSHES);
		belowLimits &= IsBelowLimit<BrushSide>(hdr, LumpType::BrushSides, MAP_MAX_BRUSHSIDES);
		belowLimits &= IsBelowLimit<Surface>(hdr, LumpType::Surfaces, MAP_MAX_SURFACES);
		belowLimits &= IsBelowLimit<Node>(hdr, LumpType::Nodes, MAP_MAX_NODES);
		belowLimits &= IsBelowLimit<Leaf>(hdr, LumpType::Leafs, MAP_MAX_LEAFS);
		belowLimits &= IsBelowLimit<Area>(hdr, LumpType::Areas, MAP_MAX_AREAS);
		belowLimits &= IsBelowLimit<uint8_t>(hdr, LumpType::Portals, MAP_MAX_PORTALS);


		if (belowLimits == false)
		{
			X_ERROR("Bsp", "%s exceeds level limits.", path.fileName());
			return false;
		}

		// time to load some data.
		LumpsLoaded = true;
//		LumpsLoaded &= LoadLump<Surface>(file, hdr.lumps[LumpType::Surfaces], data_.surfaces);
//		LumpsLoaded &= LoadLump<Vertex>(file, hdr.lumps[LumpType::Verts], data_.verts);
//		LumpsLoaded &= LoadLump<Index>(file, hdr.lumps[LumpType::Indexes], data_.indexes);
//		LumpsLoaded &= LoadLump<Area>(file, hdr.lumps[LumpType::Areas], data_.areas);

		if (LumpsLoaded == false)
		{
			X_ERROR("Bsp", "Failed to load lumps from: %s", path.fileName());
			return false;
		}
#endif

		file.close(); // not needed but makes my nipples more perky.
		return true;
	}

	return false;
}


X_NAMESPACE_END
