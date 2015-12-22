#include "stdafx.h"

#include "Dx10Render.h"
#include "../Common/Textures/XTexture.h"

#include "Util\ToggleChecker.h"

X_NAMESPACE_BEGIN(render)

namespace 
{
	static core::ToggleChecker inFontState(false);

}

bool DX11XRender::FontUpdateTexture(int texId, int nX, int nY, int USize, int VSize, byte* pData)
{
	texture::XTexture* pTex = texture::XTexture::getByID(texId);

	// should not be null.
	X_ASSERT_NOT_NULL(pTex);

	if (pTex)
	{
		pTex->updateTextureRegion(pData, nX, nY, USize, VSize, texture::Texturefmt::A8);
		return true;
	}
	return false;
}


bool DX11XRender::FontSetTexture(int texId)
{
	texture::XTexture* pTex = texture::XTexture::getByID(texId);

	// should not be null.
	X_ASSERT_NOT_NULL(pTex);

	pTex->apply(0);
	return true;
}


bool DX11XRender::FontSetRenderingState()
{
	Matrix44f* m;

	inFontState = true;

	ProMat_.Push();
	m = ProMat_.GetTop();

	float width = getWidthf();
	float height = getHeightf();
	float znear = -1e10f;
	float zfar = 1e10f;

	MatrixOrthoOffCenterRH(m, 0, width, height, 0, znear, zfar);


	ViewMat_.Push();
	ViewMat_.LoadIdentity();
	
	SetCullMode(CullMode::NONE);
	if (!SetFontShader()) {
		inFontState = false;
		ProMat_.Pop();
		ViewMat_.Pop();
		return false;
	}

	return true;
}

void DX11XRender::FontRestoreRenderingState()
{
	inFontState = false; // built in assert

	ProMat_.Pop();
	ViewMat_.Pop();
}


void DX11XRender::FontSetBlending()
{

}



X_NAMESPACE_END