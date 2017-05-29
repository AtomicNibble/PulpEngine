#pragma once

#ifndef X_MODEL_H_
#define X_MODEL_H_

#include <IModel.h>

#include "RenderMesh.h"
#include <IRender.h>

X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}

struct IoRequestBase;
struct XFileAsync;
)

X_NAMESPACE_DECLARE(engine,
	class PrimativeContext;
)

X_NAMESPACE_BEGIN(model)


class XModel
{
	typedef core::string ModelName;

public:
	XModel();
	~XModel();

	const core::string& getName(void) const;
	int32_t numLods(void) const;
	int32_t numBones(void) const;
	int32_t numBlankBones(void) const;
	int32_t numMeshTotal(void) const;
	int32_t numVerts(size_t lodIdx) const;
	bool HasLods(void) const;
	size_t lodIdxForDistance(float distance) const;


	const AABB& bounds(void) const;
	const AABB& bounds(size_t lodIdx) const;

	const Sphere& boundingSphere(size_t lodIdx) const;

	void Render(void);

	void RenderBones(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat);


	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	// can upload each lod individually.
	bool createRenderBuffersForLod(size_t idx);
	void releaseLodRenderBuffers(size_t idx);
	bool canRenderLod(size_t idx) const;

	static void RegisterVars(void);


	const LODHeader& getLod(size_t idx) const;
	const XRenderMesh& getLodRenderMesh(size_t idx) const;
	const MeshHeader& getLodMeshHdr(size_t idx) const;
	const SubMeshHeader* getMeshHead(size_t idx) const;

	void AssignDefault(XModel* pDefault);

	bool LoadModelAsync(const char* name);
	bool ReloadAsync(void);
	bool LoadModel(const char* name);
	bool LoadModel(core::XFile* pFile);

private:
	void ProcessData(char* pData);

	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* job, void* pData);
	void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* job, void* pData);


	static bool ReadHeader(ModelHeader& hdr, core::XFile* file);

private:
	int32_t id_;

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
	static Colorf model_bones_col;
};


X_NAMESPACE_END

#include "XModel.inl"

#endif // !X_MODEL_H_