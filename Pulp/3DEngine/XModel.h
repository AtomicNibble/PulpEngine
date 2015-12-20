#pragma once

#ifndef X_MODEL_H_
#define X_MODEL_H_

#include <IModel.h>
#include "Assets\AssertContainer.h"
#include "EngineBase.h"

#include <IRenderMesh.h>

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

	virtual void Render(void) X_FINAL;
	// ~IModel

	const LODHeader& getLod(size_t idx) const;
	const MeshHeader& getLodMeshHdr(size_t idx) const;
	const SubMeshHeader* getMeshHead(size_t idx) const;

	void AssignDefault(void);

	bool LoadModelAsync(const char* name);
	bool LoadModel(const char* name);
	bool LoadModel(core::XFile* pFile);

private:
	void ProcessData(char* pData);

	void IoRequestCallback(core::IFileSys* pFileSys, core::IoRequestData& request,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessHeader_job(core::V2::JobSystem* pJobSys, size_t threadIdx, core::V2::Job* job, void* pData);
	void ProcessData_job(core::V2::JobSystem* pJobSys, size_t threadIdx, core::V2::Job* job, void* pData);


	static bool ReadHeader(ModelHeader& hdr, core::XFile* file);

private:
	ModelName name_;

	model::IRenderMesh* pLodRenderMeshes_[MODEL_MAX_LODS];
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
};


X_NAMESPACE_END

#endif // !X_MODEL_H_