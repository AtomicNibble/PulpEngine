#pragma once

#include <IScriptSys.h>

X_NAMESPACE_BEGIN(engine)

namespace gui
{

    class GuiContex;
    class MenuHandler;

    class ScriptBinds_Menu : public script::IScriptBindsBase
    {
    public:
        ScriptBinds_Menu(script::IScriptSys* pSS, GuiContex& ctx);
        ~ScriptBinds_Menu();

        void bind(void);

        void setActiveHandler(MenuHandler* pMenuHandler);

    private:
        int32_t fill(script::IFunctionHandler* pH);

        int32_t open(script::IFunctionHandler* pH);
        int32_t close(script::IFunctionHandler* pH);
        int32_t back(script::IFunctionHandler* pH);

        int32_t pacifier(script::IFunctionHandler* pH);
        int32_t text(script::IFunctionHandler* pH);
        int32_t button(script::IFunctionHandler* pH);
        int32_t sliderVar(script::IFunctionHandler* pH);

        int32_t center(script::IFunctionHandler* pH);

        int32_t pushItemWidth(script::IFunctionHandler* pH);
        int32_t popItemWidth(script::IFunctionHandler* pH);


    private:
        GuiContex& ctx_;
        MenuHandler* pMenuHandler_;
    };

} // namespace gui

X_NAMESPACE_END