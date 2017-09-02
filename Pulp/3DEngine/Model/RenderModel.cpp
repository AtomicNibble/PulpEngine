#include "stdafx.h"
#include "RenderModel.h"


#include <IFont.h>

#include "Drawing\PrimativeContext.h"

X_NAMESPACE_BEGIN(model)

RenderModel::RenderModel(core::string& name) :
	XModel(name)
{

}

RenderModel::~RenderModel()
{

}

bool RenderModel::createRenderBuffersForLod(size_t idx, render::IRender* pRender)
{
	const auto& raw = hdr_.lodInfo[idx];

	return renderMeshes_[idx].createRenderBuffers(pRender, raw, hdr_.vertexFmt);
}


bool RenderModel::createSkinningRenderBuffersForLod(size_t idx, render::IRender* pRender)
{
	const auto& raw = hdr_.lodInfo[idx];

	return renderMeshes_[idx].createSkinningRenderBuffers(pRender, raw);
}

void RenderModel::releaseLodRenderBuffers(size_t idx, render::IRender* pRender)
{
	auto& renderInfo = renderMeshes_[idx];

	renderInfo.releaseRenderBuffers(pRender);
}

bool RenderModel::canRenderLod(size_t idx) const
{
	const auto& render = renderMeshes_[idx];

	return render.canRender();
}

void RenderModel::RenderBones(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Color8u col) const
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


void RenderModel::RenderBones(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Color8u col,
	const Matrix44f* pBoneMatrix, size_t num) const
{
	if (numBones() != static_cast<int32_t>(num)) {
		X_ERROR("Model", "Bone count don't match source count");
		return;
	}

	for (size_t i = 0; i < num; i++)
	{
		const Matrix44f& mat = pBoneMatrix[i];
		const Vec3f& pos = pBonePos_[i];
		const XQuatCompressedf& angle = pBoneAngles_[i];
		const uint8_t parIdx = pTagTree_[i];

		const Matrix44f& parMat = pBoneMatrix[parIdx];
		const Vec3f& parPos = pBonePos_[parIdx];
		const XQuatCompressedf& parAngle = pBoneAngles_[parIdx];

		{
			Transformf qTrans;
			qTrans.quat = angle.asQuat();
			qTrans.pos = modelMat * mat * pos;

			Transformf qTransPar;
			qTransPar.quat = parAngle.asQuat();
			qTransPar.pos = modelMat * parMat * parPos;
		
			pPrimContex->drawBone(qTransPar, qTrans, col);
		}
	}
}

void RenderModel::RenderBoneNames(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Matrix33f& view,
	Vec3f offset, float textSize, const Color8u col) const
{
	const int32_t num = numBones();
	if (!num) {
		return;
	}

	font::TextDrawContext ctx;
	ctx.col = col;
//	ctx.flags.Set(font::DrawTextFlag::CENTER);
//	ctx.flags.Set(font::DrawTextFlag::CENTER_VER);
	ctx.pFont = gEnv->pFontSys->GetDefault();
	ctx.effectId = 0;
	ctx.size = Vec2f(textSize, textSize);

	for (int32_t i = 0; i < num; i++)
	{
		const Vec3f& pos = pBonePos_[i];
		Vec3f worldPos = modelMat * pos;

		// temp hack.
		const char* pBoneName = (char*)(data_.ptr() + pTagNames_[i]);

		pPrimContex->drawText(worldPos + offset, view, ctx, pBoneName);
	}
}

void RenderModel::RenderBoneNames(engine::PrimativeContext* pPrimContex, const Matrix44f& modelMat, const Matrix33f& view,
	Vec3f offset, float textSize, const Color8u col, const Matrix44f* pBoneMatrix, size_t num) const
{
	if (numBones() != static_cast<int32_t>(num)) {
		X_ERROR("Model", "Bone count don't match source count");
		return;
	}


	font::TextDrawContext ctx;
	ctx.col = col;
//	ctx.flags.Set(font::DrawTextFlag::CENTER);
//	ctx.flags.Set(font::DrawTextFlag::CENTER_VER);
	ctx.pFont = gEnv->pFontSys->GetDefault();
	ctx.effectId = 0;
	ctx.size = Vec2f(textSize, textSize);

	for (size_t i = 0; i < num; i++)
	{
		const Vec3f& pos = pBonePos_[i];
		Vec3f worldPos = modelMat * pBoneMatrix[i] * pos;

		// temp hack.
		const char* pBoneName = (char*)(data_.ptr() + pTagNames_[i]);

		pPrimContex->drawText(worldPos + offset, view, ctx, pBoneName);
	}
}

void RenderModel::assignDefault(RenderModel* pDefault)
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
		materialNameDataSize > 0;
}


X_NAMESPACE_END