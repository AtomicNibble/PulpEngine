#include "stdafx.h"
#include "ScriptBinds_script.h"


X_NAMESPACE_BEGIN(script)

#define X_SCRIPT_REG_FUNC(func)  \
{ \
	ScriptFunction Delegate; \
	Delegate.Bind<XBinds_Script, &XBinds_Script::func>(this); \
	RegisterFunction(#func, Delegate); \
}



XBinds_Script::XBinds_Script(IScriptSys* pScriptSystem, ICore* pCore)
{
	pScriptSystem_ = pScriptSystem;
	pCore_ = pCore;

	X_ASSERT_NOT_NULL(pScriptSystem_);
	X_ASSERT_NOT_NULL(pCore_);

	XScriptableBase::Init(pScriptSystem, pCore);
	SetGlobalName("Script");

	X_SCRIPT_REG_FUNC(Load);
	X_SCRIPT_REG_FUNC(ReLoad);
	X_SCRIPT_REG_FUNC(UnLoad);

	X_SCRIPT_REG_FUNC(ListLoaded);

}

XBinds_Script::~XBinds_Script()
{

}


int XBinds_Script::Load(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	bool reload = false;
	bool raiseError = true;
	const char* fileName = nullptr;
	if (pH->GetParam(1, fileName))
	{
		if(pScriptSystem_->ExecuteFile(fileName, raiseError, reload))
			return pH->EndFunction(1);
	}
	return pH->EndFunction();
}

int XBinds_Script::ReLoad(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* fileName = nullptr;
	if (pH->GetParam(1, fileName))
	{
		if (pScriptSystem_->ReloadScript(fileName, true))
			return pH->EndFunction(1);
	}
	return pH->EndFunction();
}

int XBinds_Script::UnLoad(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* fileName = nullptr;
	if (pH->GetParam(1, fileName))
	{
		if(pScriptSystem_->UnLoadScript(fileName))
			return pH->EndFunction(1);
	}

	return pH->EndFunction();
}

int XBinds_Script::ListLoaded(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(0);

	pScriptSystem_->ListLoadedScripts();

	return pH->EndFunction();
}

X_NAMESPACE_END