#include "stdafx.h"
#include "Dx10Shader.h"

#include <IFileSys.h>
#include <ITimer.h>
#include <Util\BitUtil.h>

#include "../Common/XRender.h"
#include "Dx10Render.h"

X_NAMESPACE_BEGIN(shader)


// ===================== XShader ===========================


// D3D Effects interface
bool XShader::FXSetTechnique(const char* name)
{
	X_ASSERT_NOT_NULL(name);

	// TODO beat the living shit out of any code still using this :D 

	return FXSetTechnique(core::StrHash(name));
}

bool XShader::FXSetTechnique(const core::StrHash& name)
{
	size_t i;
	for (i = 0; i< techs.size(); i++)
	{
		if (techs[i].nameHash == name)
		{
			render::DX11XRender* rd = &render::g_Dx11D3D;

			rd->m_State.pCurShader = this;
			rd->m_State.pCurShaderTech = &techs[i];
			rd->m_State.CurShaderTechIdx = (int32)i;
			return true;
		}
	}

	X_BREAKPOINT;
	X_WARNING("Shader", "failed to find technique: %i", name);
	return false;
}

bool XShader::FXBegin(uint32 *uiPassCount, uint32 nFlags)
{
	render::DX11XRender* rd = &render::g_Dx11D3D;

	if (!rd->m_State.pCurShader || !rd->m_State.pCurShaderTech) {
		X_WARNING("Shader", "can't begin technique, none set");
		return false;
	}

	if (uiPassCount)
		*uiPassCount = 1;

	return true;
}

bool XShader::FXBeginPass(uint32 uiPass)
{
	X_UNUSED(uiPass);

	render::DX11XRender* rd = &render::g_Dx11D3D;

	if (!rd->m_State.pCurShader || !rd->m_State.pCurShaderTech) {
		X_WARNING("Shader", "fail to start pass, none set");
		return false;
	}

	// need to be able to pick a tech from what we want.
	XShaderTechnique* pTech = rd->m_State.pCurShaderTech;
	XShaderTechniqueHW* pHwTech = pTech->pCurHwTech;

	if (!pHwTech) {
		X_ERROR("Shader", "tech hW is null");
		return false;
	}

	XHWShader_Dx10* pVS = (XHWShader_Dx10*)pHwTech->pVertexShader;
	XHWShader_Dx10* pPS = (XHWShader_Dx10*)pHwTech->pPixelShader;
	XHWShader_Dx10* pGS = (XHWShader_Dx10*)pHwTech->pGeoShader;

	if (!pVS || !pPS)
	{
		X_ASSERT("Shader", "pixel / vertex shader not vaild for technique: %s", pTech->name.c_str())(pVS, pPS);
		return false;
	}

	// Pixel-shader
	if (pPS)
	{
		pPS->bind();
	}

	// Vertex-shader
	if (pVS)
	{
		pVS->bind();
	}

	// Geometry-shader
	if (pGS)
	{
		pGS->bind();
	}
	else
	{
		XHWShader_Dx10::bindGS(nullptr);
	}

	// set the state.

	render::g_Dx11D3D.SetState(pTech->state);
	render::g_Dx11D3D.SetCullMode(pTech->cullMode);

	return true;
}

bool XShader::FXCommit(const uint32 nFlags)
{

	return true;
}

bool XShader::FXEndPass()
{
	render::DX11XRender* rd = &render::g_Dx11D3D;

	rd->m_State.pCurShaderTech = nullptr;
	return true;
}

bool XShader::FXEnd()
{
	render::DX11XRender* rd = &render::g_Dx11D3D;

	rd->m_State.pCurShaderTech = nullptr;
	rd->m_State.pCurShader = nullptr;
	return true;
}


bool XShader::FXSetVSFloat(const core::StrHash& NameParam,
	const Vec4f* pVecs, uint32_t numVecs)
{
	X_ASSERT_NOT_NULL(pVecs);

	render::DX11XRender* rd = &render::g_Dx11D3D;

	if (!rd->m_State.pCurShader || !rd->m_State.pCurShaderTech) {
		X_WARNING("Shader", "fail to setVSFloat no shaer / tech set");
		return false;
	}

	XShaderTechnique* pTech = rd->m_State.pCurShaderTech;
	XShaderTechniqueHW* pHwTech = pTech->pCurHwTech;

	if (!pHwTech) {
		X_ERROR("Shader", "tech hW is null");
		return false;
	}

	XHWShader_Dx10* pVS = (XHWShader_Dx10*)pHwTech->pVertexShader;

	if (!pVS)
		return false;

	XShaderParam *pParam = pVS->getParameter(NameParam);
	if (!pParam)
		return false;

	if (pParam->numParameters != numVecs)
	{
		X_ERROR("Shader", "invalid paramater size: expected: %i, given: %i", pParam->numParameters,
			numVecs);
		return false;
	}


	pVS->setParameterRegA(pParam->bind, pParam->constBufferSlot,
		ShaderType::Vertex, pVecs, pParam->numParameters, pVS->getMaxVecs(pParam));

	return true;
}


X_NAMESPACE_END

// =========================================================

X_NAMESPACE_BEGIN(render)



bool DX11XRender::SetWorldShader()
{
	using namespace shader;

	XShader* pSh = XShaderManager::m_FixedFunction;
	uint32_t pass;

	if (!pSh)
		return false;

	core::StrHash tech("SolidTestWorld");

	if (!pSh->FXSetTechnique(tech))
		return false;

	if (!pSh->FXBegin(&pass, 0))
		return false;

	if (!pSh->FXBeginPass(pass))
		return false;

	FX_ComitParams();
	return true;
}

bool DX11XRender::SetSkyboxShader()
{
	using namespace shader;

	XShader* pSh = XShaderManager::m_FixedFunction;
	uint32_t pass;

	if (!pSh)
		return false;

	core::StrHash tech("Skybox");

	if (!pSh->FXSetTechnique(tech))
		return false;

	if (!pSh->FXBegin(&pass, 0))
		return false;

	if (!pSh->FXBeginPass(pass))
		return false;

	FX_ComitParams();
	return true;
}

bool DX11XRender::SetFFE(bool textured)
{
	using namespace shader;

	XShader* pSh = XShaderManager::m_FixedFunction;
	uint32_t pass;

	if (!pSh)
		return false;

	if (!textured)
	{
		core::StrHash tech("Solid");

		if (!pSh->FXSetTechnique(tech))
			return false;
	}
	else
	{
		core::StrHash tech("Texture");

		if (!pSh->FXSetTechnique(tech))
			return false;
	}

	if(!pSh->FXBegin(&pass, 0))
		return false;

	if(!pSh->FXBeginPass(pass))
		return false;

	FX_ComitParams();
	return true;
}

bool DX11XRender::SetFontShader()
{
	using namespace shader;

	XShader* pSh = XShaderManager::m_Font;
	uint32_t pass;

	if (!pSh)
		return false;

	core::StrHash tech("Font");
	if (!pSh->FXSetTechnique(tech))
		return false;

	if (!pSh->FXBegin(&pass, 0))
		return false;

	if (!pSh->FXBeginPass(pass))
		return false;

	FX_ComitParams();
	return true;
}


bool DX11XRender::SetZPass()
{
	using namespace shader;

	XShader* pSh = XShaderManager::m_DefferedShader;
	uint32_t pass;

	if (!pSh)
		return false;

	core::StrHash tech("WriteDeferred");
	if (!pSh->FXSetTechnique(tech))
		return false;

	if (!pSh->FXBegin(&pass, 0))
		return false;

	if (!pSh->FXBeginPass(pass))
		return false;


	shader::XHWShader_Dx10::setParams();
	shader::XHWShader_Dx10::setParamsGlobal();

	FX_ComitParams();
	return true;
}

bool DX11XRender::setGUIShader(bool textured)
{
	using namespace shader;

	XShader* pSh = XShaderManager::m_Gui;
	uint32_t pass;

	if (!pSh)
		return false;
#if 1
	// for a tech we can require certain flags like:
	// color, texture.
	// we also check the current vertext layout.
	// and match it with a input latout.

	core::StrHash tech("Fill");
	if (!pSh->FXSetTechnique(tech))
		return false;
#else
	if (textured)
	{
		core::StrHash tech("Fill#Texture");
		if (!pSh->FXSetTechnique(tech))
			return false;
	}
	else
	{
		core::StrHash tech("Fill#Color");
		if (!pSh->FXSetTechnique(tech))
			return false;
	}
#endif 

	if (!pSh->FXBegin(&pass, 0))
		return false;

	if (!pSh->FXBeginPass(pass))
		return false;


	shader::XHWShader_Dx10::setParams();
	shader::XHWShader_Dx10::setParamsGlobal();

	FX_ComitParams();
	return true;
}

void DX11XRender::FX_ComitParams(void)
{
	// call set params shiz, baby.

//	shader::XHWShader_Dx10::setParams();
//	shader::XHWShader_Dx10::setParamsGlobal();

	shader::XHWShader_Dx10::comitParamsGlobal();
	shader::XHWShader_Dx10::comitParams();


}


X_NAMESPACE_END
