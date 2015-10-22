#include "stdafx.h"
#include "XModel.h"

#include "IFileSys.h"
#include "String\Path.h"
#include "Memory\MemCursor.h"

X_NAMESPACE_BEGIN(model)

XModel::XModel()
{

}

XModel::~XModel()
{

}


// IModel
const int XModel::addRef(void)
{ 
	return XBaseAsset::addRef(); 
}

// XBaseAsset
const int XModel::release(void)
{
	const int ref = XBaseAsset::release();
	if (ref == 0) {
		X_DELETE(this, g_3dEngineArena);
	}
	return ref;
}

// ~XBaseAsset
const int XModel::forceRelease(void)
{
	for (;;) {
		if (release() <= 0)
			break;
	}
	return 0;
}


const char* XModel::getName(void) const
{
	return name_.c_str();
}

int32_t XModel::numLods(void) const
{
	return numLods_;
}

int32_t XModel::numBones(void) const
{
	return numBones_;
}

int32_t XModel::numBlankBones(void) const
{
	return numBlankBones_;
}

int32_t XModel::numMeshTotal(void) const
{
	return totalMeshNum_;
}

bool XModel::HasLods(void) const
{
	return numLods() > 1;
}

// ~IModel

const LODHeader& XModel::getLod(size_t idx) const 
{
	X_ASSERT(idx < static_cast<size_t>(numLods_), "invalid lod index")(numLods(), idx);
	return lodInfo_[idx];
}

const SubMeshHeader* XModel::getMeshHead(size_t idx) const 
{
	X_ASSERT(idx < static_cast<size_t>(numMeshTotal()), "invalid mesh index")(numMeshTotal(), idx);
	return &pMeshHeads_[idx];
}


// ==================================



//
// Loads a model into a XModel
//
//	the data is:
//	
//	||||| HEADER |||||

//  ||||| tag name IDX |||||
//  ||||| Bone Tree    |||||
//  ||||| Bone angles  |||||
//  ||||| Bone pos     |||||
//	
//
//  ||||| All MeshHeaders   |||||
//  ||||| All the verts   |||||
//  ||||| All the faces   |||||
//
// I want to basically just check the header.
// then be able to calculate the total buffer size.
// load it and set the data pointers.
//




bool XModel::LoadModel(const char* name)
{
	core::XFileScoped file;
	core::Path<char> path;
	path /= "models/";
	path /= name;
	path.setExtension(".model");

	// open the file.
	if (!file.openFile(path.c_str(), core::fileMode::READ))
		return false;

	path.removeExtension();

	name_.set(path.fileName());

	return LoadModel(file.GetFile());
}

bool XModel::LoadModel(core::XFile* file)
{
	X_ASSERT_NOT_NULL(file);

	int i, x;
	uint32_t readSize;
	ModelHeader hdr;

	if (!ReadHeader(hdr, file)) {
		X_ERROR("Model", "\"%s\" model header is invalid", name_.c_str());
		return false;
	}


#if X_DEBUG // debug only.
	// quick check that model is somewhat valid.
	// we make a few assumptions, since this is my format.
	// and if this is not right, there is a tool issue
	for (i = 0; i < hdr.numLod; i++)
	{
		LODHeader& lod = hdr.lodInfo[i];

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
	//	char* pData = X_NEW_ARRAY_ALIGNED(char, hdr.dataSize, g_3dEngineArena, "ModelBuffer", 8);
	char* pData = X_NEW_ARRAY_ALIGNED(char, hdr.dataSize, gEnv->pArena, "ModelBuffer", 8);

	if ((readSize = file->read(pData, hdr.dataSize)) != hdr.dataSize)
	{
		X_ERROR("Model", "load error failed to read %i bytes, recived: %i",
			hdr.dataSize, readSize);

		X_DELETE_ARRAY(pData, g_3dEngineArena);
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

	core::MemCursor<char> mat_name_cursor(pData, hdr.materialNameDataSize);
	core::MemCursor<char> tag_name_cursor(pData + hdr.materialNameDataSize, hdr.tagNameDataSize);
	core::MemCursor<char> cursor(pData + (hdr.dataSize - hdr.subDataSize), hdr.subDataSize);

	const size_t numBone = hdr.numBones;
	const size_t numBoneTotal = hdr.numBones + hdr.numBlankBones;

	pTagNames_ = cursor.postSeekPtr<uint16_t>(numBoneTotal);
	pTagTree_ = cursor.postSeekPtr<uint8_t>(numBone);
	pBoneAngles_ = cursor.postSeekPtr<XQuatCompressedf>(numBone);
	pBonePos_ = cursor.postSeekPtr<Vec3f>(numBone);
	pMeshHeads_ = cursor.getPtr<SubMeshHeader>();
	pData_ = pData;

	// we now have the mesh headers.
	for (i = 0; i < hdr.numLod; i++)
	{
		LODHeader& lod = hdr.lodInfo[i];

		lod.subMeshHeads = cursor.postSeekPtr<SubMeshHeader>(lod.numSubMeshes);
	}

	// ok we now need to set vert and face pointers.
	// for each Lod
	for (i = 0; i < hdr.numLod; i++)
	{
		LODHeader& lod = hdr.lodInfo[i];
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
		for (i = 0; i < hdr.numMesh; i++)
		{
			SubMeshHeader* pMesh = const_cast<SubMeshHeader*>(&pMeshHeads_[i]);
			// set the pointer.
			pMesh->materialName = mat_name_cursor.getPtr<const char>();

			while (!mat_name_cursor.isEof() && mat_name_cursor.get<char>() != '\0') {
				name.append(mat_name_cursor.getSeek<char>(), 1);
			}

			if (!mat_name_cursor.isEof())
				++mat_name_cursor;

			name.clear();
		}
	}

	// Tag name list null-term
	if (hdr.flags.IsSet(ModelFlags::LOOSE) && hdr.tagNameDataSize > 0)
	{
		core::StackString<MODEL_MAX_BONE_NAME_LENGTH> name;

		for (i = 0; i < hdr.numBones; i++)
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
	for (i = 0; i < hdr.numMesh; i++)
	{
		SubMeshHeader* pMesh = const_cast<SubMeshHeader*>(&pMeshHeads_[i]);

		pMesh->pMat = getMaterialManager()->loadMaterial(pMesh->materialName);
	}

	// copy lod info activating the data.
	core::copy_object(lodInfo_, hdr.lodInfo);

	numLods_ = hdr.numLod;
	numBones_ = hdr.numBones;
	numBlankBones_ = hdr.numBlankBones;
	totalMeshNum_ = hdr.numMesh;
	return true;
}


bool XModel::ReadHeader(ModelHeader& hdr, core::XFile* file)
{
	X_ASSERT_NOT_NULL(file);

	if (file->readObj(hdr) != sizeof(hdr))
		return false;

	// bit more info for material info.
	// even tho it's handled in isValid()
	if (hdr.materialNameDataSize == 0) {
		X_ERROR("Model", "no material data included, require atleast one material");
		return false;
	}

	return hdr.isValid();
}




/// Model functions.
bool ModelHeader::isValid(void) const
{
	if (version != MODEL_VERSION) {
		X_ERROR("Model", "model version is invalid. FileVer: %i RequiredVer: %i",
			version, MODEL_VERSION);
	}

	return version == MODEL_VERSION &&
		(numBones + numBlankBones) > 0 &&
		numLod > 0 &&
		numLod <= MODEL_MAX_LODS &&
		materialNameDataSize > 0 &&
		subDataSize > 0; // replace 0 with a real min later. TODO
}


X_NAMESPACE_END