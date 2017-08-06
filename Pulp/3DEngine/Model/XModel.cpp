#include "stdafx.h"
#include "XModel.h"
#include "ModelManager.h"


#include <Memory\MemCursor.h>
#include <IFileSys.h>
#include <IRender.h>
#include <IConsole.h>
#include <IShader.h>
#include <Threading\JobSystem2.h>

#include "Material\MaterialManager.h"
#include "Drawing\PrimativeContext.h"

X_NAMESPACE_BEGIN(model)



XModel::XModel(core::string& name) :
	name_(name)
{
	id_ = 0;
	status_ = core::LoadStatus::NotLoaded;
	pTagNames_ = nullptr;
	pTagTree_ = nullptr;
	pBoneAngles_ = nullptr;
	pBonePos_ = nullptr;
	pMeshHeads_ = nullptr;

}

XModel::~XModel()
{

}


bool XModel::createRenderBuffersForLod(size_t idx, render::IRender* pRender)
{
	const auto& raw = hdr_.lodInfo[idx];

	return renderMeshes_[idx].createRenderBuffers(pRender, raw, hdr_.vertexFmt);
}

void XModel::releaseLodRenderBuffers(size_t idx, render::IRender* pRender)
{
	auto& renderInfo = renderMeshes_[idx];

	renderInfo.releaseRenderBuffers(pRender);
}

bool XModel::canRenderLod(size_t idx) const
{
	const auto& render = renderMeshes_[idx];

	return render.canRender();
}

void XModel::RenderBones(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Color8u col) const
{
	const int32_t num = numBones();
	if (!num) {
		return;
	}

	for (int32_t i = 0; i < num; i++)
	{
		const Vec3f& pos = pBonePos_[i];
		const XQuatCompressedf& angle = pBoneAngles_[i];
		const uint8_t parIdx = pTagTree_[i];

		const Vec3f& parPos = pBonePos_[parIdx];
		const XQuatCompressedf& parAngle = pBoneAngles_[parIdx];

		{
			Transformf qTrans;
			qTrans.quat = angle.asQuat();
			qTrans.pos = modelMat * pos;

			Transformf qTransPar;
			qTransPar.quat = parAngle.asQuat();
			qTransPar.pos = modelMat * parPos;

			pPrimContex->drawBone(qTransPar, qTrans, col);
		}
	}
}


void XModel::assignDefault(XModel* pDefault)
{
	pTagNames_ = pDefault->pTagNames_;
	pTagTree_ = pDefault->pTagTree_;
	pBoneAngles_ = pDefault->pBoneAngles_;
	pBonePos_ = pDefault->pBonePos_;
	pMeshHeads_ = pDefault->pMeshHeads_;

	for (size_t i = 0; i < MODEL_MAX_LODS; i++) {
		renderMeshes_[i] = pDefault->renderMeshes_[i];
	}

	hdr_ = pDefault->hdr_;
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


void XModel::processData(ModelHeader& hdr, core::UniquePointer<uint8_t[]> data)
{
	int32_t i, x;

	core::MemCursor mat_name_cursor(data.get(), hdr.materialNameDataSize);
	core::MemCursor tag_name_cursor(data.get() + hdr.materialNameDataSize, hdr.tagNameDataSize);
	core::MemCursor cursor(data.get() + (hdr.dataSize - hdr.subDataSize), hdr.subDataSize);
	
	const size_t numBone = hdr.numBones;
	const size_t numBoneTotal = hdr.numBones + hdr.numBlankBones;

	pTagNames_ = cursor.postSeekPtr<uint16_t>(numBoneTotal);
	pTagTree_ = cursor.postSeekPtr<uint8_t>(numBone);
	pBoneAngles_ = cursor.postSeekPtr<XQuatCompressedf>(numBone);
	pBonePos_ = cursor.postSeekPtr<Vec3f>(numBone);
	pMeshHeads_ = cursor.getPtr<SubMeshHeader>();

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

		/*
		These streams are padded to 16byte align so we must seek past the
		the padding bytes manually.
		*/
		auto seekCursorToPad = [&cursor]() {

			// when the value is aligned we must work how much to seek by.
			// we do this by taking a coon to market.
			auto* pCur = cursor.getPtr<const char*>();
			auto* pAligned = core::pointerUtil::AlignTop(pCur, 16);

			const size_t diff = union_cast<size_t>(pAligned) - union_cast<size_t>(pCur);

			cursor.seekBytes(static_cast<uint32_t>(diff));
		};

		seekCursorToPad();

		// verts (always provided)
		lod.streams[VertexStream::VERT] = cursor.postSeekPtr<Vertex>(lod.numVerts);
		X_ASSERT_ALIGNMENT(lod.streams[VertexStream::VERT].as<uintptr_t>(), 16, 0);
		seekCursorToPad();

		// color
		if (lod.streamsFlag.IsSet(StreamType::COLOR)) {
			lod.streams[VertexStream::COLOR] = cursor.postSeekPtr<VertexColor>(lod.numVerts);
			X_ASSERT_ALIGNMENT(lod.streams[VertexStream::COLOR].as<uintptr_t>(), 16, 0);
			seekCursorToPad();
		}

		// normals
		if (lod.streamsFlag.IsSet(StreamType::NORMALS)) {
			lod.streams[VertexStream::NORMALS] = cursor.postSeekPtr<VertexNormal>(lod.numVerts);
			X_ASSERT_ALIGNMENT(lod.streams[VertexStream::NORMALS].as<uintptr_t>(), 16, 0);
			seekCursorToPad();
		}

		// tangents bi-normals
		if (lod.streamsFlag.IsSet(StreamType::TANGENT_BI)) {
			lod.streams[VertexStream::TANGENT_BI] = cursor.postSeekPtr<VertexTangentBi>(lod.numVerts);
			X_ASSERT_ALIGNMENT(lod.streams[VertexStream::TANGENT_BI].as<uintptr_t>(), 16, 0);
			seekCursorToPad();
		}

		// now set the sub mesh pointers.
		{
			for (x = 0; x < lod.numSubMeshes; x++)
			{
				SubMeshHeader& mesh = meshHeads[x];
				// when model is created the submesh streams have the byte offsets set.
				// so that we can just add the base address to fix them up.
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

			uint32_t size = safe_static_cast<uint32_t, size_t>(mesh.CompBinds.dataSizeTotal());

			cursor.seekBytes(size);
		}

		// index 0 is always valid, since a valid lod must
		// have a mesh.
		lod.indexes = meshHeads[0].indexes;

		X_ASSERT_ALIGNMENT(lod.indexes.as<uintptr_t>(), 16, 0);
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

			if (!mat_name_cursor.isEof()) {
				++mat_name_cursor;
			}

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
			if (!tag_name_cursor.isEof()) {
				++tag_name_cursor;
			}

			name.clear();
		}
	}

	// load the materials.
	for (i = 0; i < hdr.numMesh; i++)
	{
		SubMeshHeader& mesh = const_cast<SubMeshHeader&>(pMeshHeads_[i]);

		mesh.pMat = engine::gEngEnv.pMaterialMan_->loadMaterial(mesh.materialName);
	}

	data_ = std::move(data);
	hdr_ = hdr;
	status_ = core::LoadStatus::Complete;
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