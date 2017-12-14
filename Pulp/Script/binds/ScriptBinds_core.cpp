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

	core::ICVar* pVar = pConsole_->GetCVar(varName);

	if (pVar)
	{
		return pH->endFunction(pVar->GetInteger());
	}
	else
	{
		pScriptSys_->onScriptError("Failed to find dvar: \"%s\"", varName);
	}

	return pH->endFunction();
}

int XBinds_Core::GetDvarFloat(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* varName = nullptr;
	pH->getParam(1, varName);

	core::ICVar* pVar = pConsole_->GetCVar(varName);

	if (pVar)
	{
		return pH->endFunction(pVar->GetFloat());
	}
	else
	{
		pScriptSys_->onScriptError("Failed to find dvar: \"%s\"", varName);
	}

	return pH->endFunction();
}

int XBinds_Core::GetDvar(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* varName = nullptr;
	pH->getParam(1, varName);

	core::ICVar* pVar = pConsole_->GetCVar(varName);

	if (pVar)
	{
		if (pVar->GetType() == core::VarFlag::INT) {
			return pH->endFunction(pVar->GetInteger());
		}
		if (pVar->GetType() == core::VarFlag::FLOAT) {
			return pH->endFunction(pVar->GetFloat());
		}

		core::ICVar::StrBuf strBuf;
		if (pVar->GetType() == core::VarFlag::STRING) {
			return pH->endFunction(pVar->GetString(strBuf));
		}
		if (pVar->GetType() == core::VarFlag::COLOR) {
			return pH->endFunction(pVar->GetString(strBuf));
		}

		X_ASSERT_NOT_IMPLEMENTED();

	}
	else
	{
		pScriptSys_->onScriptError("Failed to find dvar: \"%s\"", varName);
	}

	return pH->endFunction();
}


int XBinds_Core::SetDvar(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(2);

	const char* varName = nullptr;
	pH->getParam(1, varName);

	core::ICVar* pVar = pConsole_->GetCVar(varName);

	if (pVar)
	{
		Type::Enum type = pH->getParamType(2);

		if (type == Type::Number)
		{
			float fValue = 0;
			pH->getParam(2, fValue);
			pVar->Set(fValue);
		}
		else if (type == Type::String)
		{
			const char* sValue = "";
			pH->getParam(2, sValue);
			pVar->Set(sValue);
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}
	}
	else
	{
		pScriptSys_->onScriptError("GetDvar Failed to find dvar: \"%s\"", varName);
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
		default:
			X_LOG0("Script", "Can't log type: %s", Type::ToString(value.getType()));
		break;
	}


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