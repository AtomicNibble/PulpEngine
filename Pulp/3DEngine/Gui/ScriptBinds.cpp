#include "stdafx.h"
#include "ScriptBinds.h"

#include "GuiContex.h"

#include "MenuHandler.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{

    ScriptBinds_Menu::ScriptBinds_Menu(script::IScriptSys* pSS, GuiContex& ctx, MenuHandler& menuHandler) :
        IScriptBindsBase(pSS),
        ctx_(ctx),
        menuHandler_(menuHandler)
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
        X_SCRIPT_BIND(ScriptBinds_Menu, back);

        X_SCRIPT_BIND(ScriptBinds_Menu, button);
        X_SCRIPT_BIND(ScriptBinds_Menu, sliderVar);

        // center based on item width.
        X_SCRIPT_BIND(ScriptBinds_Menu, center);

        X_SCRIPT_BIND(ScriptBinds_Menu, pushItemWidth);
        X_SCRIPT_BIND(ScriptBinds_Menu, popItemWidth);

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
        SCRIPT_CHECK_PARAMETERS(1);

        const char* pMenuName = nullptr;
        pH->getParam(1, pMenuName);

        menuHandler_.openMenu(pMenuName);

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::close(script::IFunctionHandler* pH)
    {
        menuHandler_.closeMenu();

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::back(script::IFunctionHandler* pH)
    {
        menuHandler_.back();

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
        SCRIPT_CHECK_PARAMETERS_MIN(2);

        const char* pLabel = nullptr;
        const char* pVarName = nullptr;

        pH->getParam(1, pLabel);
        pH->getParam(2, pVarName);

        float increment = 0.01f;

        if (pH->getParamCount() > 2)
        {
            pH->getParam(3, increment);
        }

        ctx_.slider(pLabel, pVarName, increment);

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::center(script::IFunctionHandler* pH)
    {
        ctx_.center();
        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::pushItemWidth(script::IFunctionHandler* pH)
    {
        SCRIPT_CHECK_PARAMETERS(1);

        float width;
        pH->getParam(width);

        ctx_.pushItemWidth(width);
        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::popItemWidth(script::IFunctionHandler* pH)
    {
        ctx_.popItemWidth();
        return pH->endFunction();
    }


} // namespace gui


X_NAMESPACE_END
