#pragma once

#include "ScriptBinds.h"

X_NAMESPACE_BEGIN(script)

class XBinds_Script : public XScriptBindsBase
{
public:
    XBinds_Script(XScriptSys* pSS);
    ~XBinds_Script();

private:
    void bind(ICore* pCore) X_FINAL;

    int load(IFunctionHandler* pH);
    int reLoad(IFunctionHandler* pH);
    int unLoad(IFunctionHandler* pH);

private:
};

X_NAMESPACE_END
