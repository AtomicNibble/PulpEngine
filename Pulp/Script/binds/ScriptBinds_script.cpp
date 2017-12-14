#include "stdafx.h"
#include "ScriptBinds_script.h"


X_NAMESPACE_BEGIN(script)

#define X_SCRIPT_REG_FUNC(func)  \
{ \
	ScriptFunction Delegate; \
	Delegate.Bind<XBinds_Script, &XBinds_Script::func>(this); \
	registerFunction(#func, Delegate); \
}



XBinds_Script::XBinds_Script(IScriptSys* pSS)
{
	init(pSS);
}

XBinds_Script::~XBinds_Script()
{

}


void XBinds_Script::init(IScriptSys* pSS)
{
	XScriptableBase::init(pSS);

	setGlobalName("Script");

	X_SCRIPT_REG_FUNC(Load);
	X_SCRIPT_REG_FUNC(ReLoad);
	X_SCRIPT_REG_FUNC(UnLoad);

}

int XBinds_Script::Load(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

//	bool reload = false;
//	bool raiseError = true;
	const char* fileName = nullptr;
	if (pH->getParam(1, fileName))
	{
	//	if (pScriptSys_->ExecuteFile(fileName, raiseError, reload)) {
	//		return pH->endFunction(1);
	//	}
	}
	return pH->endFunction();
}

int XBinds_Script::ReLoad(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* fileName = nullptr;
	if (pH->getParam(1, fileName))
	{
	//	if (pScriptSys_->ReloadScript(fileName, true)) {
	//		return pH->endFunction(1);
	//	}
	}
	return pH->endFunction();
}

int XBinds_Script::UnLoad(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	const char* fileName = nullptr;
	if (pH->getParam(1, fileName))
	{
	//	if (pScriptSys_->UnLoadScript(fileName)) {
	//		return pH->endFunction(1);
	//	}
	}

	return pH->endFunction();
}

X_NAMESPACE_END