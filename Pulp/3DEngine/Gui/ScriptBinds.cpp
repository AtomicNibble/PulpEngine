#include "stdafx.h"
#include "ScriptBinds.h"

#include "GuiContex.h"

#include "MenuHandler.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    namespace 
    {

        Color8u parseColor(script::IFunctionHandler* pH, int32_t paramIdx)
        {
            Vec4i v;

            auto type = pH->getParamType(paramIdx);
            if (type == script::Type::Table)
            {
                script::ScriptValue any;
                pH->getParamAny(paramIdx, any);

                X_ASSERT(any.getType() == script::Type::Table, "Should be table")(any.getType());

                any.pTable_->getValue("r", v.x);
                any.pTable_->getValue("g", v.y);
                any.pTable_->getValue("b", v.z);
                any.pTable_->getValue("a", v.w);
            }
            if (type == script::Type::Number)
            {
               // support ints?
                auto type1 = pH->getParamType(paramIdx + 1);
                auto type2 = pH->getParamType(paramIdx + 2);
                auto type3 = pH->getParamType(paramIdx + 3);

                if (type1 != script::Type::Number || type2 != script::Type::Number || type3 != script::Type::Number) {
                    X_ERROR("Script", "Invalid color param");
                    return Col_White;
                }

                pH->getParam(paramIdx, v.x);
                pH->getParam(paramIdx + 1, v.y);
                pH->getParam(paramIdx + 2, v.z);
                pH->getParam(paramIdx + 3, v.w);
            }

            Color8u col;
            col.r = static_cast<uint8_t>(v.x);
            col.g = static_cast<uint8_t>(v.y);
            col.b = static_cast<uint8_t>(v.z);
            col.a = static_cast<uint8_t>(v.w);
            return col;
        }

    } // namespace


    ScriptBinds_Menu::ScriptBinds_Menu(script::IScriptSys* pSS, GuiContex& ctx) :
        IScriptBindsBase(pSS),
        ctx_(ctx),
        pMenuHandler_(nullptr)
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

        X_SCRIPT_BIND(ScriptBinds_Menu, text);
        X_SCRIPT_BIND(ScriptBinds_Menu, button);
        X_SCRIPT_BIND(ScriptBinds_Menu, sliderVar);

        // center based on item width.
        X_SCRIPT_BIND(ScriptBinds_Menu, center);

        X_SCRIPT_BIND(ScriptBinds_Menu, pushItemWidth);
        X_SCRIPT_BIND(ScriptBinds_Menu, popItemWidth);

    }

    void ScriptBinds_Menu::setActiveHandler(MenuHandler* pMenuHandler)
    {
        pMenuHandler_ = pMenuHandler;
    }

    int32_t ScriptBinds_Menu::fill(script::IFunctionHandler* pH)
    {
        SCRIPT_CHECK_PARAMETERS_MIN(1);

        auto col = parseColor(pH, 1);
        ctx_.fill(col);

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::open(script::IFunctionHandler* pH)
    {
        SCRIPT_CHECK_PARAMETERS(1);

        const char* pMenuName = nullptr;
        pH->getParam(1, pMenuName);

        pMenuHandler_->openMenu(pMenuName);

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::close(script::IFunctionHandler* pH)
    {
        pMenuHandler_->closeMenu();

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::back(script::IFunctionHandler* pH)
    {
        pMenuHandler_->back();

        return pH->endFunction();
    }

    int32_t ScriptBinds_Menu::text(script::IFunctionHandler* pH)
    {
        SCRIPT_CHECK_PARAMETERS_MIN(1);

        const char* pLabel = nullptr;
        Color8u col = Col_White;

        pH->getParam(1, pLabel);

        if (pH->getParamCount() > 1)
        {
            col = parseColor(pH, 2);
        }

        ctx_.text(pLabel, pLabel + core::strUtil::strlen(pLabel), col);

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
