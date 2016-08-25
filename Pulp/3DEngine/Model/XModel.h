#pragma once

#ifndef X_MODEL_H_
#define X_MODEL_H_

#include <IModel.h>
#include "Assets\AssertContainer.h"
#include "EngineBase.h"

#include "RenderMesh.h"
// #include <IRenderMesh.h>
#include <IRender.h>

X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}
)

X_NAMESPACE_BEGIN(model)


class XModel : public IModel, public core::XBaseAsset, public engine::XEngineBase
{
	typedef core::StackString<MODEL_MAX_NAME_LENGTH> ModelName;

public:
	XModel();
	~XModel();

	// IModel
	const int addRef(void) X_OVERRIDE;
	const int release(void) X_OVERRIDE;
	const int forceRelease(void) X_OVERRIDE;

	virtual const char* getName(void) const X_FINAL;
	virtual int32_t numLods(void) const X_FINAL;
	virtual int32_t numBones(void) const X_FINAL;
	virtual int32_t numBlankBones(void) const X_FINAL;
	virtual int32_t numMeshTotal(void) const X_FINAL;
	virtual int32_t numVerts(size_t lodIdx) const X_FINAL;
	virtual bool HasLods(void) const X_FINAL;

	virtual const AABB& bounds(void) const X_FINAL;
	virtual const AABB& bounds(size_t lodIdx) const X_FINAL;

	virtual const Sphere& boundingSphere(size_t lodIdx) const X_FINAL;

	virtual void Render(void) X_FINAL;
	virtual void RenderBones(const Matrix44f& modelMat) X_FINAL;

	// ~IModel

	// can upload each lod individually.
	bool createRenderBuffersForLod(size_t idx);
	void releaseLodRenderBuffers(size_t idx);
	bool canRenderLod(size_t idx) const;

	static void RegisterVars(void);


	const LODHeader& getLod(size_t idx) const;
	const XRenderMesh& getLodRenderMesh(size_t idx) const;
	const MeshHeader& getLodMeshHdr(size_t idx) const;
	const SubMeshHeader* getMeshHead(size_t idx) const;

	void AssignDefault(void);

	bool LoadModelAsync(const char* name);
	bool ReloadAsync(void);
	bool LoadModel(const char* name);
	bool LoadModel(core::XFile* pFile);

private:
	void ProcessData(char* pData);

	void IoRequestCallback(core::IFileSys& fileSys, core::IoRequestData& request,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* job, void* pData);
	void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* job, void* pData);


	static bool ReadHeader(ModelHeader& hdr, core::XFile* file);

private:
	ModelName name_;

	XRenderMesh renderMeshes_[MODEL_MAX_LODS];
	LODHeader lodInfo_[MODEL_MAX_LODS];

	// runtime pointers.
	const uint16_t*			pTagNames_;
	const uint8_t*			pTagTree_;
	const XQuatCompressedf* pBoneAngles_;
	const Vec3f*			pBonePos_;
	// pointer to all the meshes headers for all lods
	// sotred in lod order.
	const SubMeshHeader*	pMeshHeads_;

	int32_t numLods_;
	int32_t numBones_;
	int32_t numBlankBones_;
	int32_t totalMeshNum_; // combined mesh count of all lods.

	const char* pData_;

	// used for async loading.
	ModelHeader hdr_;

private:
	static int32_t model_bones_draw;
	static Colorf model_bones_col;
};


X_NAMESPACE_END

#endif // !X_MODEL_H_