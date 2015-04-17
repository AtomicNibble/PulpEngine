#include "stdafx.h"
#include "BSPTypes.h"
#include <IFileSys.h>

#include <IBsp.h>

#include <Hashing\crc32.h>

X_USING_NAMESPACE;

using namespace bsp;

namespace
{


	void WriteAreaModel(core::XFile* file, AreaModel const* pModel)
	{
		file->writeObj(pModel->model);
		file->writeObj(pModel->meshes.ptr(),(pModel->meshes.size()));
		file->writeObj(pModel->verts.ptr(), (pModel->verts.size()));
		file->writeObj(pModel->indexes.ptr(), (pModel->indexes.size()));

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

		for (i = 0; i < areaModels.size(); i++)
		{
			AreaModel* pModel = areaModels[i];

			WriteAreaModel(file, pModel);
		}


		// update FourcCC to mark this bsp as valid.
		hdr.fourCC = BSP_FOURCC;
		hdr.numAreas = safe_static_cast<uint32_t,size_t>(areaModels.size());
		// crc the header
		hdr.datacrc32 = crc.GetCRC32((const char*)&hdr, sizeof(hdr));
		hdr.datasize = safe_static_cast<uint32_t, size_t>(file->tell() - sizeof(hdr));
		file->seek(0, core::SeekMode::SET);
		file->writeObj(hdr);

		gEnv->pFileSys->closeFile(file);
		return true;
	}
	return false;
}






// X_NAMESPACE_END