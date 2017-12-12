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
	registerFunction(#func, Delegate); }


XBinds_Core::XBinds_Core(IScriptSys* pSS, ICore* pCore)
{
	init(pSS, pCore);
}

XBinds_Core::~XBinds_Core()
{

}


void XBinds_Core::init(IScriptSys* pSS, ICore* pCore)
{
	XScriptableBase::init(pSS);

	pConsole_ = X_ASSERT_NOT_NULL(pCore->GetIConsole());
	pTimer_ = X_ASSERT_NOT_NULL(pCore->GetITimer());

	setGlobalName("Core");

	X_CORE_REG_FUNC(GetDvarInt);
	X_CORE_REG_FUNC(GetDvarFloat);
	X_CORE_REG_FUNC(GetDvar);
	X_CORE_REG_FUNC(SetDvar);

	X_CORE_REG_FUNC(Log);
	X_CORE_REG_FUNC(Warning);
	X_CORE_REG_FUNC(Error);




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

		if (type == Type::Number)
		{
			float fValue = 0;
			pH->getParam(2, fValue);
			var->Set(fValue);
		}
		else if (type == Type::String)
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
		case Type::String:
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


X_NAMESPACE_END