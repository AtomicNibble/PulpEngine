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

namespace
{
	bool ReadHeader(ModelHeader& hdr, core::XFile* file)
	{
		X_ASSERT_NOT_NULL(file);

		if (file->readObj(hdr) != sizeof(hdr)) {
			return false;
		}

		// bit more info for material info.
		// even tho it's handled in isValid()
		if (hdr.materialNameDataSize == 0) {
			X_ERROR("Model", "no material data included, require atleast one material");
			return false;
		}

		return hdr.isValid();
	}

} // namespace


XModel::XModel()
{
	pTagNames_ = nullptr;
	pTagTree_ = nullptr;
	pBoneAngles_ = nullptr;
	pBonePos_ = nullptr;
	pMeshHeads_ = nullptr;

	pData_ = nullptr;
}

XModel::~XModel()
{
	if (pData_) {
		X_DELETE_ARRAY(const_cast<char*>(pData_), g_3dEngineArena);
	}
}


bool XModel::createRenderBuffersForLod(size_t idx)
{
	const auto& raw = hdr_.lodInfo[idx];

	return renderMeshes_[idx].createRenderBuffers(gEnv->pRender, raw, hdr_.vertexFmt);
}

void XModel::releaseLodRenderBuffers(size_t idx)
{
	auto& renderInfo = renderMeshes_[idx];

	renderInfo.releaseRenderBuffers(gEnv->pRender);
}

bool XModel::canRenderLod(size_t idx) const
{
	const auto& render = renderMeshes_[idx];

	return render.canRender();
}

void XModel::RenderBones(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Color8u col) const
{
	if (numBones() > 0)
	{
		const int32_t num = numBones();

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
				qTrans.trans = modelMat * pos;

				Transformf qTransPar;
				qTransPar.quat = parAngle.asQuat();
				qTransPar.trans = modelMat * parPos;

				pPrimContex->drawBone(qTransPar, qTrans, col);
			}
		}
	}
}


void XModel::AssignDefault(XModel* pDefault)
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

void XModel::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = pRequest->getType();

	if (requestType == core::IoRequest::OPEN)
	{
		if (!pFile) {
			X_ERROR("Model", "Failed to load: %s", name_.c_str());
			return;
		}

		core::IoRequestRead read;
		read.callback.Bind<XModel, &XModel::IoRequestCallback>(this);
		read.pFile = pFile;
		read.dataSize = sizeof(hdr_);
		read.offset = 0;
		read.pBuf = &hdr_;

		fileSys.AddIoRequestToQue(read);

	}
	else if (requestType == core::IoRequest::READ)
	{
		if (!bytesTransferred) {
			X_ERROR("Model", "Failed to read model data for: %s", name_.c_str());

			core::IoRequestClose close;
			close.pFile = pFile;
			fileSys.AddIoRequestToQue(close);
			return;
		}

		core::V2::JobSystem* pJobSys = gEnv->pJobSys;
		core::V2::Job* pJob = nullptr;

		const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

		if (pReadReq->pBuf == &hdr_)
		{
			if (bytesTransferred != sizeof(hdr_)) {
				X_ERROR("Model", "Failed to read model header. Got: 0x%x need: 0x%x",
					bytesTransferred, static_cast<uint32_t>(sizeof(hdr_)));
				return;
			}

			pJob = pJobSys->CreateMemberJob<XModel>(this, &XModel::ProcessHeader_job, pFile JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		}
		else {
			
			// pData_ should not be null as we are reading into it.
			X_ASSERT_NOT_NULL(pData_);

			pJob = pJobSys->CreateMemberJob<XModel>(this, &XModel::ProcessData_job, pFile JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		}

		pJobSys->Run(pJob);
	}
}


void XModel::ProcessHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

	core::XFileAsync* pFile = static_cast<core::XFileAsync*>(pData);

	// check if the header is correct.
	if (hdr_.isValid())
	{
		// allocate buffer for the file data.
		uint32_t dataSize = hdr_.dataSize;

		char* pModelData = X_NEW_ARRAY_ALIGNED(char, hdr_.dataSize, g_3dEngineArena, "ModelBuffer", 8);

		pData_ = pModelData;

		core::IoRequestRead read;
		read.callback.Bind<XModel, &XModel::IoRequestCallback>(this);
		read.dataSize = dataSize;
		read.offset = sizeof(hdr_);
		read.pBuf = pModelData;
		read.pFile = pFile;

		gEnv->pFileSys->AddIoRequestToQue(read);
	}
	else
	{
		X_ERROR("Model", "\"%s\" model header is invalid", name_.c_str());

		core::IoRequestClose close;
		close.pFile = pFile;
		gEnv->pFileSys->AddIoRequestToQue(close);
	}
}


void XModel::ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);
	X_ASSERT_NOT_NULL(pData);

	core::XFileAsync* pFile = static_cast<core::XFileAsync*>(pData);

	ProcessData(const_cast<char*>(pData_));

	core::IoRequestClose close;
	close.pFile = pFile;
	gEnv->pFileSys->AddIoRequestToQue(close);

	// temp, unassign the render meshes so new ones get made.
	X_ASSERT_NOT_IMPLEMENTED();
}


bool XModel::LoadModelAsync(const char* name)
{
	// AssignDefault();

	core::Path<char> path;
	path /= "models";
	path /= name;
	path.setExtension(".model");

	// save the name
	name_ = path.fileName();

	// dispatch a read request baby!

	core::IoRequestOpen open;
	open.callback.Bind<XModel, &XModel::IoRequestCallback>(this);
	open.mode = core::fileMode::READ;
	open.path = path;

	gEnv->pFileSys->AddIoRequestToQue(open);

	return true;
}

bool XModel::ReloadAsync(void)
{
	core::Path<char> path;
	path /= "models";
	path /= name_.c_str();
	path.setExtension(".model");

	// dispatch a read request baby!
	core::IoRequestOpen open;
	open.callback.Bind<XModel, &XModel::IoRequestCallback>(this);
	open.mode = core::fileMode::READ;
	open.path = path;

	gEnv->pFileSys->AddIoRequestToQue(open);
	return true;
}



void XModel::ProcessData(char* pData)
{
	X_ASSERT_NOT_NULL(pData);
	X_ASSERT_ALIGNMENT(pData, 16, 0);

	int32_t i, x;

	if (!pData) {
		X_ERROR("Model", "model data is null");
		return;
	}

	core::MemCursor<char> mat_name_cursor(pData, hdr_.materialNameDataSize);
	core::MemCursor<char> tag_name_cursor(pData + hdr_.materialNameDataSize, hdr_.tagNameDataSize);
	core::MemCursor<char> cursor(pData + (hdr_.dataSize - hdr_.subDataSize), hdr_.subDataSize);

	const size_t numBone = hdr_.numBones;
	const size_t numBoneTotal = hdr_.numBones + hdr_.numBlankBones;

	pTagNames_ = cursor.postSeekPtr<uint16_t>(numBoneTotal);
	pTagTree_ = cursor.postSeekPtr<uint8_t>(numBone);
	pBoneAngles_ = cursor.postSeekPtr<XQuatCompressedf>(numBone);
	pBonePos_ = cursor.postSeekPtr<Vec3f>(numBone);
	pMeshHeads_ = cursor.getPtr<SubMeshHeader>();
	pData_ = pData;

	// we now have the mesh headers.
	for (i = 0; i < hdr_.numLod; i++)
	{
		LODHeader& lod = hdr_.lodInfo[i];

		lod.subMeshHeads = cursor.postSeekPtr<SubMeshHeader>(lod.numSubMeshes);
	}

	// ok we now need to set vert and face pointers.
	// for each Lod
	for (i = 0; i < hdr_.numLod; i++)
	{
		LODHeader& lod = hdr_.lodInfo[i];
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

			cursor.SeekBytes(static_cast<uint32_t>(diff));
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

			cursor.SeekBytes(size);
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
		for (i = 0; i < hdr_.numMesh; i++)
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
	if (hdr_.flags.IsSet(ModelFlags::LOOSE) && hdr_.tagNameDataSize > 0)
	{
		core::StackString<MODEL_MAX_BONE_NAME_LENGTH> name;

		for (i = 0; i < hdr_.numBones; i++)
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
	for (i = 0; i < hdr_.numMesh; i++)
	{
		SubMeshHeader* pMesh = const_cast<SubMeshHeader*>(&pMeshHeads_[i]);

		pMesh->pMat = engine::gEngEnv.pMaterialMan_->loadMaterial(pMesh->materialName);
	}

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