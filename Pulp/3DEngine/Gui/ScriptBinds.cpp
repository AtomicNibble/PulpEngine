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
        SCRIPT_CHECK_PARAMETERS_MIN(1);

        Color8u col;
        Vec4i v;

        auto type = pH->getParamType(1);
        if (type == script::Type::Table)
        {
            script::ScriptValue any;
            pH->getParamAny(1, any);

            X_ASSERT(any.getType() == script::Type::Table, "Should be table")(any.getType());

            any.pTable_->getValue(1, v.x);
            any.pTable_->getValue(2, v.y);
            any.pTable_->getValue(3, v.z);
            any.pTable_->getValue(4, v.w);
        }
        else
        {
            // should have 4 ints,
            pH->getParam(v.x, v.y, v.z);

            if (pH->getParamCount() > 3)
            {
                pH->getParam(4, v.w);
            }
            else
            {
                v.w = 255;
            }
        }

        col.r = static_cast<uint8_t>(v.x);
        col.g = static_cast<uint8_t>(v.y);
        col.b = static_cast<uint8_t>(v.z);
        col.a = static_cast<uint8_t>(v.w);

        ctx_.fill(col);

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
