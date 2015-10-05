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

	void WindingToTri(const XWinding* pWinding, core::Array<Vec3f>& verts)
	{
		X_ASSERT_NOT_NULL(pWinding);
		const XWinding* w = pWinding;

		int32_t i, j, numPoints;
		numPoints = w->getNumPoints();

		verts.clear();

		for (i = 2; i < numPoints; i++)
		{
			for (j = 0; j < 3; j++)
			{
				Vec3f& vert = verts.AddOne();

				if (j == 0) {
					const Vec5f vec = (*w)[0];
					vert = vec.asVec3();
				}
				else if (j == 1) {
					const Vec5f vec = (*w)[i - 1];
					vert = vec.asVec3();
				}
				else
				{
					const Vec5f vec = (*w)[i];
					vert = vec.asVec3();
				}
			}
		}
	}

	bool SavePortalWinding(const XWinding* pWinding, core::XFile* pFile)
	{
		X_ASSERT_NOT_NULL(pWinding);
		X_ASSERT_NOT_NULL(pFile);

		// we save the winding, plane and debug verts
		core::Array<Vec3f> verts(g_arena);
		Planef plane;

		pWinding->getPlane(plane);
		WindingToTri(pWinding, verts);

		if (!pWinding->SSave(pFile)) {
			X_ERROR("Lvl", "Failed to save inter portal info");
			return false;
		}

		pFile->writeObj(plane);
		pFile->writeObj(safe_static_cast<uint32_t, size_t>(verts.size()));
		if (!pFile->writeObj(verts.ptr(), verts.size())) {
			X_ERROR("Lvl", "Failed to save inter portal info");
			return false;
		}

		return true;
	}


	struct ScopedNodeInfo
	{
		ScopedNodeInfo(FileNode& node, core::XFile* pFile) :
		node_(node), pFile_(pFile) {
			X_ASSERT_NOT_NULL(pFile);
			node.offset = safe_static_cast<uint32_t,size_t>(pFile->tell());
			node.offset -= sizeof(FileHeader);
		}
		~ScopedNodeInfo() {
			node_.size = safe_static_cast<uint32_t, size_t>(pFile_->tell() - node_.offset);
			node_.size -= sizeof(FileHeader);
		}

		FileNode& node_;
		core::XFile* pFile_;
	};

}



bool LvlBuilder::save(const char* name)
{
	core::fileModeFlags mode;
	FileHeader hdr;
	core::Path<char> path;
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
				const AreaModel* pModel = &areas_[i].model;

				WriteAreaModel(file, pModel);
			}
		}
		// area portals
		if (worldEnt.interPortals.isNotEmpty())
		{
			ScopedNodeInfo node(hdr.nodes[FileNodes::AREA_PORTALS], file);

			hdr.flags.Set(LevelFileFlags::INTER_AREA_INFO);
			hdr.flags.Set(LevelFileFlags::DEBUG_PORTAL_DATA);

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
				XWinding* pWindRev = pWind->ReverseWinding(g_arena);

				if (!SavePortalWinding(pWind, file) ||
					!SavePortalWinding(pWindRev, file))
				{
					X_ERROR("Lvl", "Failed to save inter portal info");
					X_DELETE(pWindRev, g_arena);
					return false;
				}

				X_DELETE(pWindRev, g_arena);
			}
		}
		// area ent ref data
		{
			ScopedNodeInfo node(hdr.nodes[FileNodes::AREA_REFS], file);

			hdr.flags.Set(LevelFileFlags::AREA_REF_LISTS);

			uint32_t num = 0;

			for (i = 0; i < areas_.size(); i++)
			{
				const LvlArea& area = areas_[i];
				FileAreaRefHdr refHdr;
				refHdr.startIndex = num;
				refHdr.num = safe_static_cast<uint32_t, size_t>(area.entRefs.size());

				num += safe_static_cast<uint32_t, size_t>(area.entRefs.size());

				file->writeObj(refHdr);
			}

			// save each area's ref list.
			for (i = 0; i < areas_.size(); i++)
			{
				const LvlArea& area = areas_[i];

				file->writeObj(area.entRefs.ptr(), area.entRefs.size());
			}

			num = 0;

			for (i = 0; i < MAP_MAX_MULTI_REF_LISTS; i++)
			{
				FileAreaRefHdr refHdr;
				refHdr.num = safe_static_cast<uint32_t, size_t>(multiRefLists_[i].size());
				refHdr.startIndex = num; // not used.
			
				num += refHdr.num;

				file->writeObj(refHdr);
			}

			// write multi area ent ref lists.
			for (i = 0; i < MAP_MAX_MULTI_REF_LISTS; i++)
			{
				file->writeObj(multiRefLists_[i].ptr(), multiRefLists_[i].size());
			}
		}

		// models
		{
			ScopedNodeInfo node(hdr.nodes[FileNodes::STATIC_MODELS], file);

			hdr.numStaticModels = safe_static_cast<int32_t, size_t>(
				staticModels_.size());

			file->writeObj(staticModels_.ptr(), staticModels_.size());
		}


		// bsp tree
		if (worldEnt.bspTree.headnode)
		{
			ScopedNodeInfo node(hdr.nodes[FileNodes::BSP_TREE], file);

			hdr.flags.Set(LevelFileFlags::BSP_TREE);

			int32_t numNodes = worldEnt.bspTree.headnode->NumChildNodes();

			// set the header value.
			hdr.numNodes = numNodes;

			// need to write out all the nodes.
			// for none leaf nodes we will write the nodes number.
			// for leafs nodes we write the children as the area number but negative.
			worldEnt.bspTree.headnode->WriteNodes_r(planes,file);
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