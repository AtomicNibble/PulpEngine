#pragma once

#ifndef X_RENDER_MODEL_H_
#define X_RENDER_MODEL_H_

#include <Util\UniquePointer.h>


#include "RenderMesh.h"

X_NAMESPACE_DECLARE(engine,
	class PrimativeContext;
)

X_NAMESPACE_BEGIN(model)

class RenderModel : public XModel
{
	X_NO_COPY(RenderModel);
	X_NO_ASSIGN(RenderModel);

public:
	RenderModel(core::string& name);
	~RenderModel() X_OVERRIDE;

	X_INLINE const XRenderMesh& getLodRenderMesh(size_t idx) const;

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

	void assignDefault(RenderModel* pDefault);


private:
	XRenderMesh renderMeshes_[MODEL_MAX_LODS];
};


X_NAMESPACE_END

#include "RenderModel.inl"

#endif // !X_RENDER_MODEL_H_