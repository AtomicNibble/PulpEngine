#include "stdafx.h"
#include "BspLoader.h"

#include <IFileSys.h>

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
	bool IsBelowLimit(const FileLump& lump, uint32_t max, const char* str)
	{
		uint32_t num = lump.size / sizeof(T);

		if (num > max) 
		{
			X_ERROR("Bsp", "too many %s: %i max: %i",
				str, num, max);
			return false;
		}
		return true;
	}

}

Bsp::Bsp() :
	data_(g_3dEngineArena)
{

}

Bsp::~Bsp()
{

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

		// do some limit checking.
		// no pesky bsp's !
		bool belowLimits = true;
		belowLimits &= IsBelowLimit<Vertex>(hdr.lumps[LumpType::Verts], MAP_MAX_VERTS, "verts");
		belowLimits &=  IsBelowLimit<Vertex>(hdr.lumps[LumpType::Indexes], MAP_MAX_INDEXES, "indexes");

		if (belowLimits == false)
		{
			X_ERROR("Bsp", "%s exceeds level limits.", path.fileName());
			return false;
		}

		// time to load some data.
		LumpsLoaded = true;
		LumpsLoaded &= LoadLump<Surface>(file, hdr.lumps[LumpType::Surfaces], data_.surfaces);
		LumpsLoaded &= LoadLump<Vertex>(file, hdr.lumps[LumpType::Verts], data_.verts);
		LumpsLoaded &= LoadLump<Index>(file, hdr.lumps[LumpType::Indexes], data_.indexes);

		if (LumpsLoaded == false)
		{
			X_ERROR("Bsp", "Failed to load lumps from: %s", path.fileName());
			return false;
		}



		file.close(); // not needed but makes my nipples more perky.
		return true;
	}

	return false;
}


X_NAMESPACE_END
