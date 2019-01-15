#include "stdafx.h"
#include "ScriptBinds_core.h"

#include <IConsole.h>

#include <IRender.h>
#include <ITimer.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(script)

XBinds_Core::XBinds_Core(XScriptSys* pSS) :
    XScriptBindsBase(pSS)
{
}

XBinds_Core::~XBinds_Core()
{
}

void XBinds_Core::bind(ICore* pCore)
{
    pConsole_ = X_ASSERT_NOT_NULL(pCore->GetIConsole());
    pTimer_ = X_ASSERT_NOT_NULL(pCore->GetITimer());

    createBindTable();
    setGlobalName("core");

    X_SCRIPT_BIND(XBinds_Core, exec);

    X_SCRIPT_BIND(XBinds_Core, getDvarInt);
    X_SCRIPT_BIND(XBinds_Core, getDvarFloat);
    X_SCRIPT_BIND(XBinds_Core, getDvar);
    X_SCRIPT_BIND(XBinds_Core, setDvar);

    X_SCRIPT_BIND(XBinds_Core, log);
    X_SCRIPT_BIND(XBinds_Core, warning);
    X_SCRIPT_BIND(XBinds_Core, error);
}

int XBinds_Core::exec(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* pCmdStr = nullptr;
    pH->getParam(1, pCmdStr); // TODO: return view?

    pConsole_->exec(core::string_view(pCmdStr));

    return pH->endFunction();
}

int XBinds_Core::getDvarInt(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* varName = nullptr;
    pH->getParam(1, varName);

    core::ICVar* pVar = pConsole_->getCVar(core::string_view(varName));

    if (pVar) {
        return pH->endFunction(pVar->GetInteger());
    }
    else {
        pScriptSys_->onScriptError("Failed to find dvar: \"%s\"", varName);
    }

    return pH->endFunction();
}

int XBinds_Core::getDvarFloat(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* varName = nullptr;
    pH->getParam(1, varName); // todo: give me string_view?>

    core::ICVar* pVar = pConsole_->getCVar(core::string_view(varName));

    if (pVar) {
        return pH->endFunction(pVar->GetFloat());
    }
    else {
        pScriptSys_->onScriptError("Failed to find dvar: \"%s\"", varName);
    }

    return pH->endFunction();
}

int XBinds_Core::getDvar(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* varName = nullptr;
    pH->getParam(1, varName);

    core::ICVar* pVar = pConsole_->getCVar(core::string_view(varName));

    if (pVar) {
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
    else {
        pScriptSys_->onScriptError("Failed to find dvar: \"%s\"", varName);
    }

    return pH->endFunction();
}

int XBinds_Core::setDvar(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(2);

    const char* varName = nullptr;
    pH->getParam(1, varName);

    core::ICVar* pVar = pConsole_->getCVar(core::string_view(varName));

    if (pVar) {
        Type::Enum type = pH->getParamType(2);

        if (type == Type::Number) {
            float fValue = 0;
            pH->getParam(2, fValue);
            pVar->Set(fValue);
        }
        else if (type == Type::String) {
            const char* pValue = "";
            pH->getParam(2, pValue);
            pVar->Set(core::string_view(pValue));
        }
        else {
            X_ASSERT_NOT_IMPLEMENTED();
        }
    }
    else {
        pScriptSys_->onScriptError("GetDvar Failed to find dvar: \"%s\"", varName);
    }

    return pH->endFunction();
}

int XBinds_Core::log(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    ScriptValue value;
    pH->getParamAny(1, value);
    switch (value.getType()) {
        case Type::String:
            X_LOG0("Script", value.str_.pStr);
            break;
        default:
            X_LOG0("Script", "Can't log type: %s", Type::ToString(value.getType()));
            break;
    }

    return pH->endFunction();
}

int XBinds_Core::warning(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* str = nullptr;
    if (pH->getParam(1, str)) {
        X_WARNING("Script", str);
    }

    return pH->endFunction();
}

int XBinds_Core::error(IFunctionHandler* pH)
{
    SCRIPT_CHECK_PARAMETERS(1);

    const char* str = nullptr;
    if (pH->getParam(1, str)) {
        X_ERROR("Script", str);
    }

    return pH->endFunction();
}

X_NAMESPACE_END