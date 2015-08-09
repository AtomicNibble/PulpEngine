#include "stdafx.h"
#include "ModelLoader.h"
#include "ICore.h"
#include "IFileSys.h"

#include "String\StackString.h"
#include "String\Path.h"
#include <Memory\MemCursor.h>

X_NAMESPACE_BEGIN(model)

using namespace core;


ModelLoader::ModelLoader()
{
}


ModelLoader::~ModelLoader()
{
}

bool ModelLoader::LoadModel(XModel& model, const char* name)
{
	XFileScoped file;
	core::Path<char> path(name);
	path.setExtension(".model");

	// open the file.
	if (!file.openFile(path.c_str(), fileMode::READ))
		return false;

	path.removeExtension();

	model.name_.set(path.fileName());

	return LoadModel(model, file.GetFile());
}

bool ModelLoader::LoadModel(XModel& model, XFile* file)
{
	X_ASSERT_NOT_NULL(file);

	int i, x;
	uint32_t readSize;

	if (!ReadHeader(file)) {
		X_ERROR("Model", "\"%s\" model header is invalid", model.name_.c_str());
		return false;
	}


#if X_DEBUG // debug only.
	// quick check that model is somewhat valid.
	// we make a few assumptions, since this is my format.
	// and if this is not right, there is a tool issue
	for (i = 0; i < header_.numLod; i++)
	{
		LODHeader& lod = header_.lodInfo[i];

		if (lod.numSubMeshes == 0) {
			X_ERROR("Model", "model lod(%i) has no meshes defined", i);
			return false;
		}
		if (lod.numVerts == 0) {
			X_ERROR("Model", "model lod(%i) has no verts defined", i);
			return false;
		}
		if (lod.numIndexes == 0) {
			X_ERROR("Model", "model lod(%i) has no indexs defined", i);
			return false;
		}
	}
#endif // !X_DEBUG


	// ok now we just have the model + bone data.
	// which we can load all at once and just set pointers.
//	char* pData = X_NEW_ARRAY_ALIGNED(char, header_.dataSize, g_3dEngineArena, "ModelBuffer", 8);
	char* pData = X_NEW_ARRAY_ALIGNED(char, header_.dataSize, gEnv->pArena, "ModelBuffer", 8);

	if ((readSize = file->read(pData, header_.dataSize)) != header_.dataSize)
	{
		X_ERROR("Model", "load error failed to read %i bytes, recived: %i", 
			header_.dataSize, readSize);

		X_DELETE_ARRAY(pData,g_3dEngineArena);
		return false;
	}

#if X_DEBUG
	// this is a format error
	// either the loader or export has a bug.
	if (file->remainingBytes() != 0)
	{
		X_ERROR("Model", "load error, remaning bytes: %i", file->remainingBytes());
		X_DELETE_ARRAY(pData, g_3dEngineArena);
		return false;
	}
#endif // !X_DEBUG

	core::MemCursor<char> mat_name_cursor(pData, header_.materialNameDataSize);
	core::MemCursor<char> tag_name_cursor(pData + header_.materialNameDataSize, header_.tagNameDataSize);
	core::MemCursor<char> cursor(pData + (header_.dataSize - header_.subDataSize), header_.subDataSize);

	const size_t numBone = header_.numBones;
	const size_t numBoneTotal = header_.numBones + header_.numBlankBones;

	model.pTagNames_ = cursor.postSeekPtr<uint16_t>(numBoneTotal);
	model.pTagTree_ = cursor.postSeekPtr<uint8_t>(numBone);
	model.pBoneAngles_ = cursor.postSeekPtr<XQuatCompressedf>(numBone);
	model.pBonePos_ = cursor.postSeekPtr<Vec3f>(numBone);
	model.pMeshHeads_ = cursor.getPtr<SubMeshHeader>();
	model.pData_ = pData;

	// we now have the mesh headers.
	for (i = 0; i < header_.numLod; i++)
	{
		LODHeader& lod = header_.lodInfo[i];

		lod.subMeshHeads = cursor.postSeekPtr<SubMeshHeader>(lod.numSubMeshes);
	}

	// ok we now need to set vert and face pointers.
	// for each Lod
	for (i = 0; i < header_.numLod; i++)
	{
		LODHeader& lod = header_.lodInfo[i];
		// we have 3 blocks of data.
		// Verts, Faces, Binddata
		SubMeshHeader* meshHeads = lod.subMeshHeads;

		// verts (always provided)
		lod.streams[VertexStream::VERT] = cursor.postSeekPtr<Vertex>(lod.numVerts);

		// color
		if (lod.streamsFlag.IsSet(StreamType::COLOR)) {
			lod.streams[VertexStream::COLOR] = cursor.postSeekPtr<VertexColor>(lod.numVerts);
		}

		// normals
		if (lod.streamsFlag.IsSet(StreamType::NORMALS)) {
			lod.streams[VertexStream::NORMALS] = cursor.postSeekPtr<VertexNormal>(lod.numVerts);
		}

		// tangents bi-normals
		if (lod.streamsFlag.IsSet(StreamType::TANGENT_BI)) {
			lod.streams[VertexStream::TANGENT_BI] = cursor.postSeekPtr<VertexTangentBi>(lod.numVerts);
		}

		// now set the sub mesh pointers.
		{
			for (x = 0; x < lod.numSubMeshes; x++)
			{
				SubMeshHeader& mesh = meshHeads[x];
				// when model is created the submesh streams have the byte offsets set.
				// so that we can just app the base address to fix them up.
				mesh.streams[VertexStream::VERT] += lod.streams[VertexStream::VERT];
				mesh.streams[VertexStream::COLOR] += lod.streams[VertexStream::COLOR];
				mesh.streams[VertexStream::NORMALS] += lod.streams[VertexStream::NORMALS];
				mesh.streams[VertexStream::TANGENT_BI] += lod.streams[VertexStream::TANGENT_BI];
			}
		}

		// indexes
		for (x = 0; x < lod.numSubMeshes; x++)
		{
			SubMeshHeader& mesh = meshHeads[x];
			mesh.indexes = cursor.postSeekPtr<Index>(mesh.numIndexes);
		}

		// BindData
		for (x = 0; x < lod.numSubMeshes; x++)
		{
			SubMeshHeader& mesh = meshHeads[x];

			uint32_t size = (uint32_t)mesh.CompBinds.dataSizeTotal();

			cursor.SeekBytes(size);
		}

		// index 0 is always valid, since a valid lod must
		// have a mesh.
		lod.indexes = meshHeads[0].indexes;
	}

	

	// even tho the names are stored at the top of the file we process them now.
	// so that i can set name pointers in the MeshHeaders for convience :D !

	// Material name list null-term
	{
		core::StackString<engine::MTL_MATERIAL_MAX_LEN> name;
		for (i = 0; i < header_.numMesh; i++)
		{		
			SubMeshHeader* pMesh = const_cast<SubMeshHeader*>(&model.pMeshHeads_[i]);
			// set the pointer.
			pMesh->materialName = mat_name_cursor.getPtr<const char>();

			while (!mat_name_cursor.isEof() && mat_name_cursor.get<char>() != '\0')	{
				name.append(mat_name_cursor.getSeek<char>(), 1);
			}

			if (!mat_name_cursor.isEof())
				++mat_name_cursor;

			name.clear();
		}
	}

	// Tag name list null-term
	if (header_.flags.IsSet(ModelFlags::LOOSE) && header_.tagNameDataSize > 0)
	{
		core::StackString<MODEL_MAX_BONE_NAME_LENGTH> name;

		for (i = 0; i < header_.numBones; i++)
		{
			while (!tag_name_cursor.isEof() && tag_name_cursor.get<char>() != '\0') {
				name.append(tag_name_cursor.getSeek<char>(), 1);
			}

			// TODO: add these names to the string table.
			if (!tag_name_cursor.isEof())
				++tag_name_cursor;

			name.clear();
		}
	}

	// load the materials.
	for (i = 0; i < header_.numMesh; i++)
	{
		SubMeshHeader* pMesh = const_cast<SubMeshHeader*>(&model.pMeshHeads_[i]);

		pMesh->pMat = getMaterialManager()->loadMaterial(pMesh->materialName);
	}

	// copy lod info activating the data.
	core::copy_object(model.lodInfo_, header_.lodInfo);

	model.numLods_ = header_.numLod;
	model.numBones_ = header_.numBones;
	model.numBlankBones_ = header_.numBlankBones;
	model.totalMeshNum_ = header_.numMesh;
	return true;
}


bool ModelLoader::ReadHeader(XFile* file)
{
	X_ASSERT_NOT_NULL(file);

	if (file->readObj(header_) != sizeof(header_))
		return false;

	// bit more info for material info.
	// even tho it's handled in isValid()
	if (header_.materialNameDataSize == 0) {
		X_ERROR("Model", "no material data included, require atleast one material");
		return false;
	}

	return header_.isValid();
}




/// Model functions.
bool ModelHeader::isValid(void) const
{
	if (version != MODEL_VERSION) {
		X_ERROR("Model", "model version is invalid. FileVer: %i RequiredVer: %i", 
			version, MODEL_VERSION);
	}

	return version == MODEL_VERSION &&
#if X_DEBUG == 0
		numBones > 0 &&
#endif // !X_DEBUG
		numLod > 0 &&
		numLod <= MODEL_MAX_LODS &&
		materialNameDataSize > 0 &&
		subDataSize > 0; // replace 0 with a real min later. TODO
}

X_NAMESPACE_END
