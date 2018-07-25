#include "stdafx.h"
#include "ScriptBinds.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{

    ScriptBinds_Menu::ScriptBinds_Menu(script::IScriptSys* pSS) :
        IScriptBindsBase(pSS)
    {
    }

    ScriptBinds_Menu::~ScriptBinds_Menu()
    {
    }

    void ScriptBinds_Menu::bind(void)
    {
        createBindTable();
        setGlobalName("gui");

        X_SCRIPT_BIND(ScriptBinds_Menu, Text);
    }

    int32_t ScriptBinds_Menu::Text(script::IFunctionHandler* pH)
    {
        // so i want to draw text.
        // how to get prim?

        return pH->endFunction();
    }


} // namespace gui


X_NAMESPACE_END
