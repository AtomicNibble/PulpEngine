#pragma once

#ifndef X_MODEL_H_
#define X_MODEL_H_

#include <Util\UniquePointer.h>

#include <IModel.h>
#include <IRender.h>
#include <IPhysics.h>

#include "RenderMesh.h"


X_NAMESPACE_DECLARE(engine,
	class PrimativeContext;
)

X_NAMESPACE_BEGIN(model)


class XModel : public IModel
{
	X_NO_COPY(XModel);
	X_NO_ASSIGN(XModel);

public:
	XModel(core::string& name);
	~XModel() X_OVERRIDE;

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	X_INLINE core::LoadStatus::Enum getStatus(void) const;
	X_INLINE bool isLoaded(void) const;
	X_INLINE bool loadFailed(void) const;
	X_INLINE void setStatus(core::LoadStatus::Enum status);

	X_INLINE const core::string& getName(void) const;
	X_INLINE int32_t numLods(void) const;
	X_INLINE int32_t numBones(void) const;
	X_INLINE int32_t numBlankBones(void) const;
	X_INLINE int32_t numMeshTotal(void) const;
	X_INLINE int32_t numVerts(size_t lodIdx) const;
	X_INLINE bool hasLods(void) const;
	X_INLINE bool hasPhys(void) const;
	X_INLINE bool isAnimated(void) const;
	X_INLINE size_t lodIdxForDistance(float distance) const;

	X_INLINE const AABB& bounds(void) const;
	X_INLINE const AABB& bounds(size_t lodIdx) const;
	X_INLINE const Sphere& boundingSphere(size_t lodIdx) const;

	X_INLINE const LODHeader& getLod(size_t idx) const;
	X_INLINE const XRenderMesh& getLodRenderMesh(size_t idx) const;
	X_INLINE const MeshHeader& getLodMeshHdr(size_t idx) const;
	X_INLINE const SubMeshHeader& getMeshHead(size_t idx) const;


	// can upload each lod individually.
	bool createRenderBuffersForLod(size_t idx, render::IRender* pRender);
	bool createSkinningRenderBuffersForLod(size_t idx, render::IRender* pRender);
	void releaseLodRenderBuffers(size_t idx, render::IRender* pRender);
	bool canRenderLod(size_t idx) const;

	void RenderBones(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Color8u col) const;
	void RenderBones(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Color8u col, const Matrix44f* pBoneMatrix, size_t num) const;
	void RenderBoneNames(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Matrix33f& view,
		Vec3f offset, float textSize, const Color8u col) const;
	void RenderBoneNames(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Matrix33f& view,
		Vec3f offset, float textSize, const Color8u col, const Matrix44f* pBoneMatrix, size_t num) const;

	void assignDefault(XModel* pDefault);

	void processData(ModelHeader& hdr, core::UniquePointer<uint8_t[]> data);

	void addPhysToActor(physics::ActorHandle actor);

private:
	int32_t id_;
	core::string name_;
	XRenderMesh renderMeshes_[MODEL_MAX_LODS];
	
	core::LoadStatus::Enum status_;
	uint8_t _pad[3];

	// runtime pointers.
	const uint16_t*			pTagNames_;
	const uint8_t*			pTagTree_;
	const XQuatCompressedf* pBoneAngles_;
	const Vec3f*			pBonePos_;
	// pointer to all the meshes headers for all lods
	// sotred in lod order.
	const SubMeshHeader*	pMeshHeads_;

	core::UniquePointer<uint8_t[]> data_;
	ModelHeader hdr_;
};


X_NAMESPACE_END

#include "XModel.inl"

#endif // !X_MODEL_H_