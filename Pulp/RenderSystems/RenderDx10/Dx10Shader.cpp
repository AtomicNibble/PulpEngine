#include "stdafx.h"
#include "Dx10Shader.h"

#include <IFileSys.h>
#include <ITimer.h>
#include <Util\BitUtil.h>

#include "../Common/XRender.h"
#include "Dx10Render.h"


#include "../Common/Textures/XTexture.h"

X_NAMESPACE_BEGIN(shader)


// ===================== XShader ===========================

InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt)
{
	switch (fmt)
	{
		case VertexFormat::P3F_T3F:
			return InputLayoutFormat::POS_UV;
		case VertexFormat::P3F_T2S:
			return InputLayoutFormat::POS_UV;

		case VertexFormat::P3F_T2S_C4B:
			return InputLayoutFormat::POS_UV_COL;
		case VertexFormat::P3F_T2S_C4B_N3F:
			return InputLayoutFormat::POS_UV_COL_NORM;
		case VertexFormat::P3F_T2S_C4B_N3F_TB3F:
			return InputLayoutFormat::POS_UV_COL_NORM_TAN_BI;

		case VertexFormat::P3F_T2S_C4B_N10:
			return InputLayoutFormat::POS_UV_COL_NORM;
		case VertexFormat::P3F_T2S_C4B_N10_TB10:
			return InputLayoutFormat::POS_UV_COL_NORM_TAN_BI;

		case VertexFormat::P3F_T2F_C4B:
			return InputLayoutFormat::POS_UV_COL;

		case VertexFormat::P3F_T4F_C4B_N3F:
			return InputLayoutFormat::POS_UV2_COL_NORM;

		case VertexFormat::Num:
			X_ASSERT_UNREACHABLE();
			return InputLayoutFormat::Invalid;
#if X_DEBUG
		default:
			X_ASSERT_UNREACHABLE();
			return InputLayoutFormat::POS_UV;
#else
			X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
	}
}

ILFlags IlFlagsForVertexFormat(const VertexFormat::Enum fmt)
{
	switch (fmt)
	{
		case VertexFormat::P3F_T3F:
			return ILFlags();
		case VertexFormat::P3F_T2S:
			return ILFlags();

		case VertexFormat::P3F_T2S_C4B:
			return ILFlag::Color;
		case VertexFormat::P3F_T2S_C4B_N3F:
			return ILFlag::Color | ILFlag::Normal;
		case VertexFormat::P3F_T2S_C4B_N3F_TB3F:
			return ILFlag::Color | ILFlag::Normal | ILFlag::BiNormal;

		case VertexFormat::P3F_T2S_C4B_N10:
			return ILFlag::Color | ILFlag::Normal;
		case VertexFormat::P3F_T2S_C4B_N10_TB10:
			return ILFlag::Color | ILFlag::Normal | ILFlag::BiNormal;

		case VertexFormat::P3F_T2F_C4B:
			return ILFlag::Color;

		case VertexFormat::P3F_T4F_C4B_N3F:
			return ILFlag::Color | ILFlag::Normal;

#if X_DEBUG
		default:
			X_ASSERT_UNREACHABLE();
			return ILFlags();
#else
			X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
	}
}



// D3D Effects interface
bool XShader::FXSetTechnique(const char* name, const TechFlags flag)
{
	X_ASSERT_NOT_NULL(name);

	// TODO beat the living shit out of any code still using this :D 

	return FXSetTechnique(core::StrHash(name), flag);
}

bool XShader::FXSetTechnique(const core::StrHash& name, const TechFlags flags)
{
	size_t i;
	for (i = 0; i< techs.size(); i++)
	{
		if (techs[i].nameHash == name)
		{
			render::DX11XRender* rd = &render::g_Dx11D3D;

			XShaderTechnique& tech = techs[i];

			rd->m_State.pCurShader = this;
			rd->m_State.pCurShaderTech = &tech;
			rd->m_State.CurShaderTechIdx = (int32)i;


			// work out which HW tech to used based on the flags :D !
			if (tech.hwTechs.size() > 1)
			{
				// ok so for every we have ones for all flags.
				for (auto& it : tech.hwTechs)
				{
					if (it.techFlags == flags)
					{
						tech.pCurHwTech = &it;
						return true;
					}
				}

				tech.resetCurHWTech();
			}
#if X_DEBUG
			else if (tech.hwTechs.isEmpty())
			{
				X_ERROR("Shader", "tech has no hw techs");
				return false;
			}
#endif // !X_DEBUG
			else
			{
			// not needed if only one.
			//	tech.resetCurHWTech();
			}

			return true;
		}
	}

	X_BREAKPOINT;
	X_WARNING("Shader", "failed to find technique: %i", name);
	return false;
}

bool XShader::FXBegin(uint32 *pPassCountOut, uint32 flags)
{
	X_UNUSED(flags);
	render::DX11XRender* rd = &render::g_Dx11D3D;

	if (!rd->m_State.pCurShader || !rd->m_State.pCurShaderTech) {
		X_WARNING("Shader", "can't begin technique, none set");
		return false;
	}

	if (pPassCountOut) {
		*pPassCountOut = 1;
	}

	return true;
}

bool XShader::FXBeginPass(uint32 passIdx)
{
	X_UNUSED(passIdx);

	render::DX11XRender* rd = &render::g_Dx11D3D;
	render::RenderState& state = rd->m_State;

	if (!state.pCurShader || !state.pCurShaderTech) {
		X_WARNING("Shader", "fail to start pass, none set");
		return false;
	}

	// need to be able to pick a tech from what we want.
	XShaderTechnique* pTech = state.pCurShaderTech;
	XShaderTechniqueHW* pHwTech = pTech->pCurHwTech;

	if (!pHwTech) {
		X_ERROR("Shader", "tech hW is null");
		return false;
	}

	// check the vertex format fits.
	InputLayoutFormat::Enum requiredIlFmt = ILfromVertexFormat(state.CurrentVertexFmt);
	if (pHwTech->IlFmt != requiredIlFmt)
	{
		// find one that fits.
		TechFlags requiredFlags = pHwTech->techFlags;

		if (pTech->hwTechs.size() > 1) {
			for (auto& it : pTech->hwTechs) {
				if (it.techFlags == requiredFlags 
					&& it.IlFmt == requiredIlFmt) {
					pHwTech = &it;
				}
			}
		}
		else
		{
			// humm potentially the correct input will be set later.
			// i will warm and fix any code that sets it after tho.
		//	X_WARNING("Shader", "could not find a hardware tech that fits the current vertexFmt");
		}
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

bool XShader::FXCommit(const uint32 flags)
{
	X_UNUSED(flags);
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

	if (pParam->numParameters != safe_static_cast<int,uint32_t>(numVecs))
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

	XShader* pSh = XShaderManager::m_WordShader;
	uint32_t pass;


	texture::XTexture::applyDefault();

	if (!pSh)
		return false;

	core::StrHash tech("Solid");

	if (!pSh->FXSetTechnique(tech))
		return false;

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T4F_C4B_N3F)))
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

bool DX11XRender::SetFFE(shader::VertexFormat::Enum vertFmt, bool textured)
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

	if (FAILED(FX_SetVertexDeclaration(vertFmt)))
		return false;

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

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
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

#if 0
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
		core::StrHash tech("Fill");
		if (!pSh->FXSetTechnique(tech, TechFlag::Textured))
			return false;
	}
	else
	{
		core::StrHash tech("Fill");
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
