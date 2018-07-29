#pragma once

#ifndef X_SCRIPT_BINDS_CORE_H_
#define X_SCRIPT_BINDS_CORE_H_

X_NAMESPACE_DECLARE(core,
                    struct IConsole;
                    struct ITimer;)

#ifdef DrawText
#undef DrawText
#endif // !DrawText

#include "ScriptBinds.h"

X_NAMESPACE_BEGIN(script)

class XScriptableBase;
class XScriptSys;

class XBinds_Core : public XScriptBindsBase
{
public:
    XBinds_Core(XScriptSys* pSS);
    ~XBinds_Core();

private:
    void bind(ICore* pCore) X_FINAL;

    int exec(IFunctionHandler* pH);

    int getDvarInt(IFunctionHandler* pH);
    int getDvarFloat(IFunctionHandler* pH);
    int getDvar(IFunctionHandler* pH);
    int setDvar(IFunctionHandler* pH);

    int log(IFunctionHandler* pH);
    int warning(IFunctionHandler* pH);
    int error(IFunctionHandler* pH);

private:
    core::IConsole* pConsole_;
    core::ITimer* pTimer_;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_CORE_H_