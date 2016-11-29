#include "stdafx.h"
#include "XModel.h"

#include "IFileSys.h"
#include "Memory\MemCursor.h"

#include <IRender.h>
#include <IRenderAux.h>
#include <IConsole.h>
#include <IShader.h>

#include <Threading\JobSystem2.h>


X_NAMESPACE_BEGIN(model)



int32_t XModel::model_bones_draw;
Colorf XModel::model_bones_col;


XModel::XModel()
{
	pTagNames_ = nullptr;
	pTagTree_ = nullptr;
	pBoneAngles_ = nullptr;
	pBonePos_ = nullptr;
	pMeshHeads_ = nullptr;

	numLods_ = 0;
	numBones_ = 0;
	numBlankBones_ = 0;
	totalMeshNum_ = 0;

	pData_ = nullptr;
}

XModel::~XModel()
{
	if (pData_) {
		X_DELETE_ARRAY(const_cast<char*>(pData_), gEnv->pArena);
	}
}


// IModel

const core::string& XModel::getName(void) const
{
	return name_;
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

int32_t XModel::numVerts(size_t lodIdx) const
{
	X_ASSERT(lodIdx < static_cast<size_t>(numLods_), "invalid lod index")(numLods(), lodIdx);

	return lodInfo_[lodIdx].numVerts;
}

bool XModel::HasLods(void) const
{
	return numLods() > 1;
}


const AABB& XModel::bounds(void) const
{
	return hdr_.boundingBox;
}

const AABB& XModel::bounds(size_t lodIdx) const
{
	return hdr_.lodInfo[lodIdx].boundingBox;
}

const Sphere& XModel::boundingSphere(size_t lodIdx) const
{
	return hdr_.lodInfo[lodIdx].boundingSphere;
}

void XModel::Render(void)
{
	// this will be removed soon.
	// there will be no way to global render somthing must be slapped in a list.

}


void XModel::RenderBones(const Matrix44f& modelMat)
{
	using namespace render;

	// should move this out tbh.
	// save a call for every model when this is disabled.
	// check should be done once for a collection
	if (!model_bones_draw) {
		return;
	}

	if (numBones() > 0)
	{
		render::IRenderAux* pAux = this->getRender()->GetIRenderAuxGeo();

		XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;
		flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOff);
		flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOff);
		flags.SetFillMode(AuxGeom_FillMode::FillModeWireframe);

		pAux->setRenderFlags(flags);

		int32_t num = numBones();

		Color8u col = model_bones_col;

		for (int32_t i = 0; i < num; i++)
		{
			const Vec3f& pos = pBonePos_[i];
			const XQuatCompressedf& angle = pBoneAngles_[i];
			const uint8_t parIdx = pTagTree_[i];

			const Vec3f& parPos = pBonePos_[parIdx];
			const XQuatCompressedf& parAngle = pBoneAngles_[parIdx];

			{
				QuatTransf qTrans;
				qTrans.quat = angle.asQuat();
				qTrans.trans = modelMat * pos;
			

				QuatTransf qTransPar;
				qTransPar.quat = parAngle.asQuat();
				qTransPar.trans = modelMat * parPos;

				pAux->drawBone(qTransPar, qTrans, col);
			}
		}
	}
}

// ~IModel

bool XModel::createRenderBuffersForLod(size_t idx)
{
	const auto& raw = lodInfo_[idx];

	return renderMeshes_[idx].createRenderBuffers(pRender_, hdr_.vertexFmt, raw);
}

void XModel::releaseLodRenderBuffers(size_t idx)
{
	auto& renderInfo = renderMeshes_[idx];

	renderInfo.releaseRenderBuffers(pRender_);
}

bool XModel::canRenderLod(size_t idx) const
{
	const auto& render = renderMeshes_[idx];

	return render.canRender();
}

void XModel::RegisterVars(void)
{	
	ADD_CVAR_REF_NO_NAME(model_bones_draw, 0, 0, 1, 
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draw model bones. 0=off 1=on");

	ADD_CVAR_REF_COL_NO_NAME(model_bones_col, Color(0.7f, 0.5f, 0.f, 1.f),
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Model bone color");
}

const LODHeader& XModel::getLod(size_t idx) const 
{
	X_ASSERT(idx < static_cast<size_t>(numLods_), "invalid lod index")(numLods(), idx);
	return lodInfo_[idx];
}

const XRenderMesh& XModel::getLodRenderMesh(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(numLods_), "invalid lod index")(numLods(), idx);
	return renderMeshes_[idx];
}

const MeshHeader& XModel::getLodMeshHdr(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(numLods_), "invalid lod index")(numLods(), idx);
	return lodInfo_[idx];
}

const SubMeshHeader* XModel::getMeshHead(size_t idx) const 
{
	X_ASSERT(idx < static_cast<size_t>(numMeshTotal()), "invalid mesh index")(numMeshTotal(), idx);
	return &pMeshHeads_[idx];
}


void XModel::AssignDefault(void)
{
	XModel* pDefault = static_cast<XModel*>(getModelManager()->getDefaultModel());

	pTagNames_ = pDefault->pTagNames_;
	pTagTree_ = pDefault->pTagTree_;
	pBoneAngles_ = pDefault->pBoneAngles_;
	pBonePos_ = pDefault->pBonePos_;
	pMeshHeads_ = pDefault->pMeshHeads_;


	numLods_ = pDefault->numLods_;
	numBones_ = pDefault->numBones_;
	numBlankBones_ = pDefault->numBlankBones_;
	totalMeshNum_ = pDefault->totalMeshNum_;


	for (size_t i = 0; i < MODEL_MAX_LODS; i++) {
		renderMeshes_[i] = pDefault->renderMeshes_[i];
	}
	for (size_t i = 0; i < MODEL_MAX_LODS; i++) {
		lodInfo_[i] = pDefault->lodInfo_[i];
	}

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

void XModel::IoRequestCallback(core::IFileSys& fileSys, core::IoRequestData& request,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = request.getType();

	if (requestType == core::IoRequest::OPEN)
	{
		if (!pFile) {
			X_ERROR("Model", "Failed to load: %s", name_.c_str());
			return;
		}

		core::IoRequestData req;
		req.setType(core::IoRequest::READ);
		req.callback.Bind<XModel, &XModel::IoRequestCallback>(this);

		core::IoRequestRead& read = req.readInfo;
		read.pFile = pFile;
		read.dataSize = sizeof(hdr_);
		read.offset = 0;
		read.pBuf = &hdr_;

		fileSys.AddIoRequestToQue(req);

	}
	else if (requestType == core::IoRequest::READ)
	{
		if (!bytesTransferred) {
			X_ERROR("Model", "Failed to read model data for: %s", name_.c_str());

			core::IoRequestData req;
			req.setType(core::IoRequest::CLOSE);
			req.closeInfo.pFile = pFile;
			pFileSys_->AddIoRequestToQue(req);
			return;
		}

		core::V2::JobSystem* pJobSys = gEnv->pJobSys;
		core::V2::Job* pJob = nullptr;

		if (request.readInfo.pBuf == &hdr_) 
		{
			if (bytesTransferred != sizeof(hdr_)) {
				X_ERROR("Model", "Failed to read model header. Got: 0x%x need: 0x%x",
					bytesTransferred, static_cast<uint32_t>(sizeof(hdr_)));
				return;
			}

			pJob = pJobSys->CreateMemberJob<XModel>(this, &XModel::ProcessHeader_job, pFile);
		}
		else {
			
			// pData_ should not be null as we are reading into it.
			X_ASSERT_NOT_NULL(pData_);

			pJob = pJobSys->CreateMemberJob<XModel>(this, &XModel::ProcessData_job, pFile);
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

		char* pModelData = X_NEW_ARRAY_ALIGNED(char, hdr_.dataSize, gEnv->pArena, "ModelBuffer", 8);

		pData_ = pModelData;

		core::IoRequestData req;
		req.setType(core::IoRequest::READ);
		req.callback.Bind<XModel, &XModel::IoRequestCallback>(this);

		core::IoRequestRead& read = req.readInfo;
		read.dataSize = dataSize;
		read.offset = sizeof(hdr_);
		read.pBuf = pModelData;
		read.pFile = pFile;

		pFileSys_->AddIoRequestToQue(req);
	}
	else
	{
		X_ERROR("Model", "\"%s\" model header is invalid", name_.c_str());

		core::IoRequestData req;
		req.setType(core::IoRequest::CLOSE);
		req.closeInfo.pFile = pFile;
		pFileSys_->AddIoRequestToQue(req);
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

	core::IoRequestData req;
	req.setType(core::IoRequest::CLOSE);
	req.closeInfo.pFile = pFile;
	pFileSys_->AddIoRequestToQue(req);

	// temp, unassign the render meshes so new ones get made.
	X_ASSERT_NOT_IMPLEMENTED();
}


bool XModel::LoadModelAsync(const char* name)
{
	AssignDefault();

	core::Path<char> path;
	path /= "models";
	path /= name;
	path.setExtension(".model");

	// save the name
	name_ = path.fileName();

	// dispatch a read request baby!
	core::IoRequestData req;
	req.setType(core::IoRequest::OPEN);
	req.callback.Bind<XModel, &XModel::IoRequestCallback>(this);

	core::IoRequestOpen& open = req.openInfo;
	open.mode = core::fileMode::READ;
	open.name = path.c_str();

	pFileSys_->AddIoRequestToQue(req);

	return true;
}

bool XModel::ReloadAsync(void)
{
	core::Path<char> path;
	path /= "models";
	path /= name_.c_str();
	path.setExtension(".model");

	// dispatch a read request baby!
	core::IoRequestData req;
	req.setType(core::IoRequest::OPEN);
	req.callback.Bind<XModel, &XModel::IoRequestCallback>(this);

	core::IoRequestOpen& open = req.openInfo;
	open.mode = core::fileMode::READ;
	open.name = path.c_str();

	pFileSys_->AddIoRequestToQue(req);
	return true;
}


bool XModel::LoadModel(const char* name)
{
	core::XFileScoped file;
	core::Path<char> path;
	path /= "models";
	path /= name;
	path.setExtension(".model");

	// open the file.
	if (!file.openFile(path.c_str(), core::fileMode::READ)) {
		return false;
	}

	path.removeExtension();

	name_ = name;

	return LoadModel(file.GetFile());
}

bool XModel::LoadModel(core::XFile* file)
{
	X_ASSERT_NOT_NULL(file);

	if (!ReadHeader(hdr_, file)) {
		X_ERROR("Model", "\"%s\" model header is invalid", name_.c_str());
		return false;
	}

#if X_DEBUG // debug only.

	// quick check that model is somewhat valid.
	// we make a few assumptions, since this is my format.
	// and if this is not right, there is a tool issue
	for (uint32_t i = 0; i < hdr_.numLod; i++)
	{
		LODHeader& lod = hdr_.lodInfo[i];

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
	//	char* pData = X_NEW_ARRAY_ALIGNED(char, hdr_.dataSize, g_3dEngineArena, "ModelBuffer", 8);
	char* pData = X_NEW_ARRAY_ALIGNED(char, hdr_.dataSize, gEnv->pArena, "ModelBuffer", 16); 
	size_t readSize;

	if ((readSize = file->read(pData, hdr_.dataSize)) != hdr_.dataSize)
	{
		X_ERROR("Model", "load error failed to read %i bytes, recived: %i",
			hdr_.dataSize, readSize);

		X_DELETE_ARRAY(pData, g_3dEngineArena);
		return false;
	}

#if X_DEBUG
	// this is a format error
	// either the loader or export has a bug.
	if (file->remainingBytes() != 0)
	{
		X_ERROR("Model", "load error, remaning bytes: %" PRIuS, file->remainingBytes());
		X_DELETE_ARRAY(pData, g_3dEngineArena);
		return false;
	}
#endif // !X_DEBUG

	ProcessData(pData);

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

		pMesh->pMat = getMaterialManager()->loadMaterial(pMesh->materialName);
	}

	// copy lod info activating the data.
	core::copy_object(lodInfo_, hdr_.lodInfo);

	numLods_ = hdr_.numLod;
	numBones_ = hdr_.numBones;
	numBlankBones_ = hdr_.numBlankBones;
	totalMeshNum_ = hdr_.numMesh;
}


bool XModel::ReadHeader(ModelHeader& hdr, core::XFile* file)
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