#include "stdafx.h"
#include "BSPFile.h"

#include <IFileSys.h>

#include <IBsp.h>

#include <Hashing\crc32.h>

X_NAMESPACE_BEGIN(bsp)

namespace
{

	template<typename T>
	void SaveLump(core::XFile* file, bsp::FileLump& lump, const core::Array<T>& vec)
	{
		uint32_t len = safe_static_cast<uint32_t, size_t>(vec.size() * sizeof(T));

		lump.offset = safe_static_cast<uint32_t,size_t>(file->tell());
		lump.size = len;

		if (file->write(vec.ptr(), len) != len)
			core::zero_object(lump);
	}

}


BSPFile::BSPFile()
{

}



bool BSPFile::save(const BSPData& data, const char* name)
{
	core::fileModeFlags mode;
	FileHeader hdr;
	core::Path path;

	mode.Set(core::fileMode::WRITE);
	mode.Set(core::fileMode::RECREATE);
	mode.Set(core::fileMode::RANDOM_ACCESS); // we do a seek.

	core::zero_object(hdr);
	hdr.fourCC = BSP_FOURCC_INVALID;
	hdr.version = BSP_VERSION;
	hdr.datacrc32 = 0; 
	hdr.modified = core::dateTimeStampSmall::systemDateTime();

	path.append(name);
	path.setExtension(bsp::BSP_FILE_EXTENSION);

	core::Crc32 crc;

	core::XFile* file = gEnv->pFileSys->openFile(path.c_str(), mode);
	if (file)
	{
		file->writeObj(hdr);

		SaveLump<Area>(file, hdr.lumps[LumpType::Areas], data.areas);
		SaveLump<Surface>(file, hdr.lumps[LumpType::Surfaces], data.surfaces);
		SaveLump<Vertex>(file, hdr.lumps[LumpType::Verts], data.verts);
		SaveLump<Index>(file, hdr.lumps[LumpType::Indexes], data.indexes);

		// update FourcCC to mark this bsp as valid.
		hdr.fourCC = BSP_FOURCC;
		// we crc32 just the lumps.
		hdr.datacrc32 = crc.GetCRC32((const char*)hdr.lumps, sizeof(hdr.lumps));


		file->seek(0, core::SeekMode::SET);
		file->writeObj(hdr);

		gEnv->pFileSys->closeFile(file);
		return true;
	}
	return false;
}











#if 0
namespace 
{

	template<typename T>
	void SaveLump(core::XFile* file, BSPLump& lump, const std::vector<T>& vec)
	{
		uint32_t len = safe_static_cast<uint32_t,size_t>(vec.size() * sizeof(T));

		lump.offset = 0; // file->
		lump.size = len;

		if (file->write(vec.data(), len) != len)
			core::zero_object(lump);
	}

	void SaveLump(core::XFile* file, BSPLump& lump, const std::string& str)
	{
		uint32_t len = safe_static_cast<uint32_t, size_t>(str.size());

		lump.offset = 0; // file->
		lump.size = len;

		if (file->write(str.data(), len) != len)
			core::zero_object(lump);
	}


	template<typename T>
	void LoadLump(core::XFileScoped& file, const BSPLump& lump, std::vector<T>& vec)
	{
		uint32_t num = lump.size / sizeof(T);

		vec.resize(num);

		// seek if needed.
		file.seek(lump.offset, core::SeekMode::SET);
		file.read(vec.data(), num);
	}

	void LoadLump(core::XFileScoped& file, const BSPLump& lump, std::string& str)
	{
		uint32_t num = lump.size;

		if (num == 0) // prevent out of bounds index. []
			return;

		str.resize(num);

		// seek if needed.
		file.seek(lump.offset, core::SeekMode::SET);
		file.read(&str[0], num);
	}
}

// --------------------------------------------------------------

BSPFile::BSPFile(BSPData* pData) :
	pData_(pData)
{
	X_ASSERT_NOT_NULL(pData);
}

bool BSPFile::save(core::Path& path)
{
	BSPHeader hdr;
	core::zero_object(hdr);

	hdr.fourCC = BSP_FOURCC_INVALID;
	hdr.version = BSP_VERSION;
	hdr.datacrc32 = 0; // might crc32 the data for reload checks.
	hdr.modified = core::dateTimeStampSmall::systemDateTime();

	core::fileModeFlags mode;
	mode.Set(core::fileMode::WRITE);
	mode.Set(core::fileMode::RECREATE);

	// ensure correct extension is set.
	path.setExtension( BSP_FILE_EXTENSION );

	core::XFile* file = gEnv->pFileSys->openFile(path.c_str(), mode);
	if (file)
	{
		file->writeObj(hdr);

		// we need to write the blocks :D
		// order we save in is not critical for the file to load correct.
		// but we should avoid seeks, to not slow down none memory file reads.

//		SaveLump(file, hdr.lumps[LumpTypes::Entities], entities);
		// mats
		// planes
/*		SaveLump<BSPNode>(file, hdr.lumps[LumpTypes::Nodes], nodes);
		SaveLump<BSPLeaf>(file, hdr.lumps[LumpTypes::Leafs], leafs);
		SaveLump<int>(file, hdr.lumps[LumpTypes::LeafSurfaces], leafSurfIndex);
		SaveLump<int>(file, hdr.lumps[LumpTypes::LeafBrushes], leafBrushIndex);
		SaveLump<BSPModel>(file, hdr.lumps[LumpTypes::Models], models);
		SaveLump<BSPBrush>(file, hdr.lumps[LumpTypes::Brushes], brushes);
		SaveLump<BSPBrushSide>(file, hdr.lumps[LumpTypes::BrushSides], brushSides);
		*/
	
//		SaveLump<BSPVertex>(file, hdr.lumps[LumpTypes::Verts], vertices);
//		SaveLump<int>(file, hdr.lumps[LumpTypes::Faces], vertexIndicies);

		/*		SaveLump<BSPSurface>(file, hdr.lumps[LumpTypes::Surfaces], surfaces);
		SaveLump<BSPColor>(file, hdr.lumps[LumpTypes::LightMaps], lightMaps);
		SaveLump<BSPColor>(file, hdr.lumps[LumpTypes::LightVolumes], lightGrid);
*/

		// We need to rewrite the header, now we have the block sizes set.
		// We also update the fourCC to make the bsp valid. :)
		hdr.fourCC = BSP_FOURCC;
		file->seek(0, core::SeekMode::SET);
		file->writeObj(hdr);

		gEnv->pFileSys->closeFile(file);
		return true;
	}
	return false;
}


bool BSPFile::load(core::Path& path)
{
#if 0
	BSPHeader hdr;
	core::fileModeFlags mode;
	core::XFileScoped file; // use a scoped file so we can return on errors.
	
	core::zero_object(hdr);
	mode.Set(core::fileMode::READ);

	// ensure correct extension is set.
	path.setExtension(BSP_FILE_EXTENSION);

	if (file.openFile(path.c_str(), mode))
	{
		file.readObj(hdr);

		// hey slut are you a valid file O>O !?
		if (!hdr.isValid())
		{
			if (hdr.fourCC == BSP_FOURCC_INVALID)
				X_ERROR("bsp", "bsp file header is corrupt");
			else
				X_ERROR("bsp", "not a valid bsp file");
			return false;
		}

		// what version you be rolling up in my grill?
		if (hdr.version != BSP_VERSION)
		{
			X_ERROR("bsp", "bsp version is invalid. provided: %i, required: %i",
				hdr.version, BSP_VERSION);
		}


	//	LoadLump(file, hdr.lumps[bsp::LumpTypes::Entities], nullptr);


		LoadLump(file, hdr.lumps[LumpTypes::Entities], entities);
		// mats
		// planes
		LoadLump<BSPNode>(file, hdr.lumps[LumpTypes::Nodes], nodes);
		LoadLump<BSPLeaf>(file, hdr.lumps[LumpTypes::Leafs], leafs);
		LoadLump<int>(file, hdr.lumps[LumpTypes::LeafSurfaces], leafSurfIndex);
		LoadLump<int>(file, hdr.lumps[LumpTypes::LeafBrushes], leafBrushIndex);
		LoadLump<BSPModel>(file, hdr.lumps[LumpTypes::Models], models);
		LoadLump<BSPBrush>(file, hdr.lumps[LumpTypes::Brushes], brushes);
		LoadLump<BSPBrushSide>(file, hdr.lumps[LumpTypes::BrushSides], brushSides);
		LoadLump<BSPVertex>(file, hdr.lumps[LumpTypes::Verts], vertices);
		LoadLump<int>(file, hdr.lumps[LumpTypes::Faces], vertexIndicies);
		LoadLump<BSPSurface>(file, hdr.lumps[LumpTypes::Surfaces], surfaces);
		LoadLump<BSPColor>(file, hdr.lumps[LumpTypes::LightMaps], lightMaps);
		LoadLump<BSPColor>(file, hdr.lumps[LumpTypes::LightVolumes], lightGrid);

	}
#endif
	return false;
}


#endif
X_NAMESPACE_END