#include "stdafx.h"
#include "ScriptBinds_core.h"

#include <IConsole.h>

#include <IRender.h>
#include <ITimer.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(script)

#define X_CORE_REG_FUNC(func)  \
{	ScriptFunction Delegate; \
	Delegate.Bind<XBinds_Core, &XBinds_Core::func>(this); \
	RegisterFunction(#func, Delegate); }


XBinds_Core::XBinds_Core()
{

}

XBinds_Core::~XBinds_Core()
{

}


void XBinds_Core::Init(IScriptSys* pSS, ICore* pCore, int paramIdOffset)
{
	XScriptableBase::Init(pSS, pCore, paramIdOffset);

	X_ASSERT_NOT_NULL(pCore->GetIConsole());
	X_ASSERT_NOT_NULL(pCore->GetITimer());

	pConsole_ = pCore->GetIConsole();
	pTimer_ = pCore->GetITimer();

	XScriptableBase::Init(pSS, pCore);
	SetGlobalName("Core");

	X_CORE_REG_FUNC(GetDvarInt);
	X_CORE_REG_FUNC(GetDvarFloat);
	X_CORE_REG_FUNC(GetDvar);
	X_CORE_REG_FUNC(SetDvar);

	X_CORE_REG_FUNC(Log);
	X_CORE_REG_FUNC(Warning);
	X_CORE_REG_FUNC(Error);

	X_CORE_REG_FUNC(DrawLine);
	X_CORE_REG_FUNC(DrawLine2D);
	X_CORE_REG_FUNC(DrawText);
	X_CORE_REG_FUNC(DrawCone);


	X_CORE_REG_FUNC(GetCurrTime);
	X_CORE_REG_FUNC(GetCurrAsyncTime);
	X_CORE_REG_FUNC(GetFrameTime);
	X_CORE_REG_FUNC(GetTimeScale);
}


int XBinds_Core::GetDvarInt(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* varName = nullptr;
	pH->getParam(1, varName);

	core::ICVar* var = pConsole_->GetCVar(varName);

	if (var)
	{
		return pH->endFunction(var->GetInteger());
	}
	else
	{
		pScriptSys_->onScriptError("Failed to fine dvar: \"%s\"", varName);
	}

	return pH->endFunction();
}

int XBinds_Core::GetDvarFloat(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* varName = nullptr;
	pH->getParam(1, varName);

	core::ICVar* var = pConsole_->GetCVar(varName);

	if (var)
	{
		return pH->endFunction(var->GetFloat());
	}
	else
	{
		pScriptSys_->onScriptError("Failed to fine dvar: \"%s\"", varName);
	}

	return pH->endFunction();
}

int XBinds_Core::GetDvar(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* varName = nullptr;
	pH->getParam(1, varName);

	core::ICVar* var = pConsole_->GetCVar(varName);

	if (var)
	{
		if (var->GetType() == core::VarFlag::INT)
			return pH->endFunction(var->GetInteger());
		if (var->GetType() == core::VarFlag::FLOAT)
			return pH->endFunction(var->GetFloat());

		{
			core::ICVar::StrBuf strBuf;
			if (var->GetType() == core::VarFlag::STRING)
				return pH->endFunction(var->GetString(strBuf));
			if (var->GetType() == core::VarFlag::COLOR)
				return pH->endFunction(var->GetString(strBuf));
		}
	}
	else
	{
		pScriptSys_->onScriptError("Failed to fine dvar: \"%s\"", varName);
	}

	return pH->endFunction();
}


int XBinds_Core::SetDvar(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(2);

	const char* varName = nullptr;
	pH->getParam(1, varName);

	core::ICVar* var = pConsole_->GetCVar(varName);

	if (var)
	{
		Type::Enum type = pH->getParamType(2);

		if (type == Type::NUMBER)
		{
			float fValue = 0;
			pH->getParam(2, fValue);
			var->Set(fValue);
		}
		else if (type == Type::STRING)
		{
			const char* sValue = "";
			pH->getParam(2, sValue);
			var->Set(sValue);
		}
	}
	else
	{
		pScriptSys_->onScriptError("GetDvar Failed to fine dvar: \"%s\"", varName);
	}

	return pH->endFunction();
}



int XBinds_Core::Log(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	ScriptValue value;
	pH->getParamAny(1,value);
	switch (value.getType())
	{
		case Type::STRING:

		X_LOG0("Script", value.str_.pStr);

		break;
	}

//	const char* str = nullptr;
//	if (pH->getParam(1, str))
//	{
//		X_LOG0("Script", str);
//	}

	return pH->endFunction();
}

int XBinds_Core::Warning(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* str = nullptr;
	if (pH->getParam(1, str))
	{
		X_WARNING("Script", str);
	}

	return pH->endFunction();
}

int XBinds_Core::Error(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* str = nullptr;
	if (pH->getParam(1, str))
	{
		X_ERROR("Script", str);
	}
	
	return pH->endFunction();
}

// -----------------------------------------------

int XBinds_Core::DrawLine(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(6);

	Vec3f v0, v1;
	Color fcol;

	pH->getParam(1,v0);
	pH->getParam(2,v1);

	pH->getParam(3, fcol.r);
	pH->getParam(4, fcol.g);
	pH->getParam(5, fcol.b);
	pH->getParam(6, fcol.a);

	Color8u col(fcol);
#if 1
	X_ASSERT_UNREACHABLE();
#else
	render::IRenderAux* pRenderAuxGeom = gEnv->pRender->GetIRenderAuxGeo();

	pRenderAuxGeom->setRenderFlags(render::AuxGeom_Defaults::Def3DRenderflags);
	pRenderAuxGeom->drawLine(v0, col, v1, col);
#endif

	return pH->endFunction();
}

int XBinds_Core::DrawLine2D(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(6);

	Vec3f v0, v1;
	Color fcol;

	pH->getParam(1, v0);
	pH->getParam(2, v1);

	const float c_Normalize2Dx(1.0f / 800.0f);
	const float c_Normalize2Dy(1.0f / 600.0f);
	v0.x *= c_Normalize2Dx;
	v0.y *= c_Normalize2Dy;
	v1.x *= c_Normalize2Dx;
	v1.y *= c_Normalize2Dy;

	pH->getParam(3, fcol.r);
	pH->getParam(4, fcol.g);
	pH->getParam(5, fcol.b);
	pH->getParam(6, fcol.a);

	Color8u col(fcol);

#if 1
	X_ASSERT_UNREACHABLE();
#else
	render::IRenderAux* pRenderAuxGeom = gEnv->pRender->GetIRenderAuxGeo();
	render::XAuxGeomRenderFlags flags(render::AuxGeom_Defaults::Def2DPRenderflags);

	if (fcol.r < 1.f)
	{
		flags.SetAlphaBlendMode(render::AuxGeom_AlphaBlendMode::AlphaBlended);
	}

	pRenderAuxGeom->setRenderFlags(flags);
	pRenderAuxGeom->drawLine(v0, col, v1, col);
#endif

	return pH->endFunction();
}

int XBinds_Core::DrawText(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(9);

	font::IFontSys* pFontSys = gEnv->pFontSys;

	if (!pFontSys)
		return pH->endFunction();


	float x = 0;
	float y = 0;
	const char *text = "";
	const char *fontName = "default";
	float size = 16;
	float r = 1;
	float g = 1;
	float b = 1;
	float a = 1;

	pH->getParam(1, x);
	pH->getParam(2, y);
	pH->getParam(3, text);
	pH->getParam(4, fontName);
	pH->getParam(5, size);
	pH->getParam(6, r);
	pH->getParam(7, g);
	pH->getParam(8, b);
	pH->getParam(9, a);

	font::IFont* pFont = pFontSys->GetFont(fontName);

	if (!pFont)
	{
		return pH->endFunction();
	}


	font::TextDrawContext ctx;
	ctx.SetColor(Color(r, g, b, a));
	ctx.SetSize(Vec2f(size, size));
//	ctx.SetProportional(true);
//	ctx.SetSizeIn800x600(true);

	X_ASSERT_NOT_IMPLEMENTED();
//	pFont->DrawString(x, y, text, ctx);

	return pH->endFunction();
}

int XBinds_Core::DrawCone(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(8);

	Vec3f pos, dir;
	float radius, height;
	Color fcol;

	pH->getParam(1, pos);
	pH->getParam(2, dir);

	pH->getParam(3, radius);
	pH->getParam(4, height);

	pH->getParam(5, fcol.r);
	pH->getParam(6, fcol.g);
	pH->getParam(7, fcol.b);
	pH->getParam(8, fcol.a);

	Color8u col(fcol);

#if 1
	X_ASSERT_UNREACHABLE();
#else
	render::IRenderAux* pRenderAuxGeom = gEnv->pRender->GetIRenderAuxGeo();
	render::XAuxGeomRenderFlags flags(render::AuxGeom_Defaults::Def3DRenderflags);

	pRenderAuxGeom->drawCone(pos, dir, radius, height, col);
#endif

	return pH->endFunction();
}

// -----------------------------------------------


// alot of these should be removed.
// and we should not really expose all these values it's confusing.
// we just want to give them: real time, and unscaled time.


int XBinds_Core::GetCurrTime(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(0);
	X_ASSERT_NOT_IMPLEMENTED();

	float fTime = 0.f; // pTimer_->GetCurrTime();
	return pH->endFunction(fTime);
}

int XBinds_Core::GetCurrAsyncTime(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(0);
	X_ASSERT_NOT_IMPLEMENTED();

	float fTime = 0.f; // pTimer_->GetAsyncCurTime();
	return pH->endFunction(fTime);
}

int XBinds_Core::GetFrameTime(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(0);
	X_ASSERT_NOT_IMPLEMENTED();

	float fTime = 0.f; // pTimer_->GetFrameTime();
	return pH->endFunction(fTime);
}

int XBinds_Core::GetTimeScale(IFunctionHandler *pH)
{
	SCRIPT_CHECK_PARAMETERS(0);
	X_ASSERT_NOT_IMPLEMENTED();

	float scale = 0.f; // pTimer_->GetTimeScale();
	return pH->endFunction(scale);
}


X_NAMESPACE_END