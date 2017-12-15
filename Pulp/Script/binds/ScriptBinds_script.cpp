#include "stdafx.h"
#include "ScriptBinds_script.h"


X_NAMESPACE_BEGIN(script)


XBinds_Script::XBinds_Script(XScriptSys* pSS) :
	XScriptBindsBase(pSS)
{
	
}

XBinds_Script::~XBinds_Script()
{

}


void XBinds_Script::bind(ICore* pCore)
{
	X_UNUSED(pCore);

	createBindTable();
	setGlobalName("Script");

	X_SCRIPT_BIND(XBinds_Script, Load);
	X_SCRIPT_BIND(XBinds_Script, ReLoad);
	X_SCRIPT_BIND(XBinds_Script, UnLoad);

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