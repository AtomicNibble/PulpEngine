#include "stdafx.h"

#include "LevelBuilder.h"

#include <IFileSys.h>
#include <Ilevel.h>

#include <Hashing\crc32.h>

X_USING_NAMESPACE;

using namespace level;

namespace
{


	void WriteAreaModel(core::XFile* file, AreaModel const* pModel)
	{
		file->writeObj(pModel->model);
		file->writeObj(pModel->meshes.ptr(),(pModel->meshes.size()));

		// write the streams.
		core::Array<level::Vertex>::ConstIterator it = pModel->verts.begin();
		core::Array<level::Vertex>::ConstIterator end = pModel->verts.end();
		for (; it != end; ++it) {
			file->writeObj(it->pos);
			file->writeObj(it->texcoord);
		}
		it = pModel->verts.begin();
		for (; it != end; ++it) {
			file->writeObj(it->color);
		}
		it = pModel->verts.begin();
		for (; it != end; ++it) {
			file->writeObj(it->normal);
		}

	//	file->writeObj(pModel->verts.ptr(), (pModel->verts.size()));
		file->writeObj(pModel->faces.ptr(), (pModel->faces.size()));
	}
}




bool LvlBuilder::save(const char* name)
{
	core::fileModeFlags mode;
	FileHeader hdr;
	core::Path path;
	size_t i;

	mode.Set(core::fileMode::WRITE);
	mode.Set(core::fileMode::RECREATE);
	mode.Set(core::fileMode::RANDOM_ACCESS); // we do a seek.

	core::zero_object(hdr);
	hdr.fourCC = LVL_FOURCC_INVALID;
	hdr.version = LVL_VERSION;
	hdr.datacrc32 = 0; 
	hdr.modified = core::dateTimeStampSmall::systemDateTime();

	hdr.numStrings = safe_static_cast<uint32_t,size_t>(stringTable_.numStrings());

	path.append(name);
	path.setExtension(level::LVL_FILE_EXTENSION);

	core::Crc32 crc;

	core::XFile* file = gEnv->pFileSys->openFile(path.c_str(), mode);
	if (file)
	{
		file->writeObj(hdr);

		size_t stringTableStart = file->tell();

		// write string table.
		stringTable_.SSave(file);

		size_t stringTableEnd = file->tell();

		for (i = 0; i < areas_.size(); i++)
		{
			AreaModel* pModel = &areas_[i].model;

			WriteAreaModel(file, pModel);
		}

		size_t dataEnd = file->tell();


		// update FourcCC to mark this bsp as valid.
		hdr.fourCC = LVL_FOURCC;
		hdr.numAreas = safe_static_cast<uint32_t,size_t>(areas_.size());
		// crc the header
		hdr.datacrc32 = crc.GetCRC32((const char*)&hdr, sizeof(hdr));


		hdr.stringDataSize = safe_static_cast<uint32_t, size_t>(
			stringTableEnd - stringTableStart);
		hdr.datasize = safe_static_cast<uint32_t, size_t>(dataEnd -
			stringTableEnd);

		hdr.totalDataSize = hdr.stringDataSize + hdr.datasize;

		file->seek(0, core::SeekMode::SET);
		file->writeObj(hdr);

		gEnv->pFileSys->closeFile(file);
		return true;
	}
	return false;
}






// X_NAMESPACE_END