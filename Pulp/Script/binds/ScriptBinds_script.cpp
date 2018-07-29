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
    setGlobalName("script");

    X_SCRIPT_BIND(XBinds_Script, load);
    X_SCRIPT_BIND(XBinds_Script, reLoad);
    X_SCRIPT_BIND(XBinds_Script, unLoad);
}

int XBinds_Script::load(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* pFileName = nullptr;
    if (pH->getParam(1, pFileName)) {
        if (pScriptSys_->onInclude(pFileName)) {
            return pH->endFunction(1);
        }
    }

    return pH->endFunction();
}

int XBinds_Script::reLoad(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* fileName = nullptr;
    if (pH->getParam(1, fileName)) {
        //	if (pScriptSys_->ReloadScript(fileName, true)) {
        //		return pH->endFunction(1);
        //	}
    }
    return pH->endFunction();
}

int XBinds_Script::unLoad(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* fileName = nullptr;
    if (pH->getParam(1, fileName)) {
        //	if (pScriptSys_->UnLoadScript(fileName)) {
        //		return pH->endFunction(1);
        //	}
    }

    return pH->endFunction();
}

X_NAMESPACE_END