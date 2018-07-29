#include "stdafx.h"
#include "ScriptBinds.h"

#include "GuiContex.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{

    ScriptBinds_Menu::ScriptBinds_Menu(script::IScriptSys* pSS, GuiContex& ctx) :
        IScriptBindsBase(pSS),
        ctx_(ctx)
    {
    }

    ScriptBinds_Menu::~ScriptBinds_Menu()
    {
    }

    void ScriptBinds_Menu::bind(void)
    {
        createBindTable();
        setGlobalName("ui");

        X_SCRIPT_BIND(ScriptBinds_Menu, fill);
        X_SCRIPT_BIND(ScriptBinds_Menu, open);
        X_SCRIPT_BIND(ScriptBinds_Menu, close);
        X_SCRIPT_BIND(ScriptBinds_Menu, button);
        X_SCRIPT_BIND(ScriptBinds_Menu, sliderVar);
    }

    int32_t ScriptBinds_Menu::fill(script::IFunctionHandler* pH)
    {
        pCtx_->fill(Col_Salmon);

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::open(script::IFunctionHandler* pH)
    {

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::close(script::IFunctionHandler* pH)
    {

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::button(script::IFunctionHandler* pH)
    {
        SCRIPT_CHECK_PARAMETERS(1);

        const char* pLabel = nullptr;
        pH->getParam(1, pLabel);

        bool res = ctx_.button(pLabel);

        return pH->endFunction(res);
    }

    int32_t ScriptBinds_Menu::sliderVar(script::IFunctionHandler* pH)
    {
        SCRIPT_CHECK_PARAMETERS(2);

        const char* pLabel = nullptr;
        const char* pVarName = nullptr;

        pH->getParam(1, pLabel);
        pH->getParam(2, pVarName);

        ctx_.slider(pLabel, pVarName);

        return pH->endFunction();
    }



} // namespace gui


X_NAMESPACE_END
