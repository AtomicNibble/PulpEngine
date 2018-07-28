#pragma once

#include <IScriptSys.h>

X_NAMESPACE_BEGIN(engine)

namespace gui
{

    class GuiContex;

    class ScriptBinds_Menu : public script::IScriptBindsBase
    {
    public:
        ScriptBinds_Menu(script::IScriptSys* pSS);
        ~ScriptBinds_Menu();

        void bind(void);

    private:
        int32_t fill(script::IFunctionHandler* pH);

        int32_t open(script::IFunctionHandler* pH);
        int32_t close(script::IFunctionHandler* pH);

        int32_t button(script::IFunctionHandler* pH);
        
        int32_t sliderVar(script::IFunctionHandler* pH);

    private:
        GuiContex* pCtx_;
    };

} // namespace gui

X_NAMESPACE_END