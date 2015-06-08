#include "stdafx.h"

#include "LvlBuilder.h"

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


	struct ScopedNodeInfo
	{
		ScopedNodeInfo(FileNode& node, core::XFile* pFile) :
		node_(node), pFile_(pFile) {
			X_ASSERT_NOT_NULL(pFile);
			node.offset = pFile->tell();
		}
		~ScopedNodeInfo() {
			node_.size = pFile_->tell() - node_.offset;
		}

		FileNode& node_;
		core::XFile* pFile_;
	};

}



bool LvlBuilder::save(const char* name)
{
	core::fileModeFlags mode;
	FileHeader hdr;
	core::Path path;
	size_t i;

	if (entities_.isEmpty()) {
		X_ERROR("Lvl", "Failed to save lvl file has zero entities");
		return false;
	}

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

		LvlEntity& worldEnt = entities_[0];

		// string table
		{
			ScopedNodeInfo node(hdr.nodes[FileNodes::STRING_TABLE], file);
			if (!stringTable_.SSave(file)) {
				X_ERROR("Lvl", "Failed to save string table");
				return false;
			}
		}
		// areas
		{
			ScopedNodeInfo node(hdr.nodes[FileNodes::AREAS], file);

			for (i = 0; i < areas_.size(); i++)
			{
				AreaModel* pModel = &areas_[i].model;

				WriteAreaModel(file, pModel);
			}
		}
		// area portals
		if (worldEnt.interPortals.isNotEmpty())
		{
			ScopedNodeInfo node(hdr.nodes[FileNodes::AREA_PORTALS], file);

			hdr.numinterAreaPortals = safe_static_cast<uint32_t, size_t>(
				worldEnt.interPortals.size());

			// need to write the area's 
			// and the winding.
			for (const auto& iap : worldEnt.interPortals)
			{
				file->writeObj(iap.area0);
				file->writeObj(iap.area1);

				X_ASSERT_NOT_NULL(iap.pSide);
				X_ASSERT_NOT_NULL(iap.pSide->pWinding);

				const XWinding* pWind = iap.pSide->pWinding;

				if (!pWind->SSave(file)) {
					X_ERROR("Lvl", "Failed to save inter portal info");
					return false;
				}
			}

		}

		// bsp tree
		{


		}


		// update FourcCC to mark this bsp as valid.
		hdr.fourCC = LVL_FOURCC;
		hdr.numAreas = safe_static_cast<uint32_t,size_t>(areas_.size());
		// crc the header
		hdr.datacrc32 = crc.GetCRC32((const char*)&hdr, sizeof(hdr));

		for (uint32_t i = 0; i < FileNodes::ENUM_COUNT; i++)
		{
			hdr.totalDataSize += hdr.nodes[i].size;
		}

		file->seek(0, core::SeekMode::SET);
		file->writeObj(hdr);

		gEnv->pFileSys->closeFile(file);
		return true;
	}
	return false;
}






// X_NAMESPACE_END