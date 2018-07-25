#pragma once

#include <IScriptSys.h>

X_NAMESPACE_BEGIN(engine)

namespace gui
{

    class ScriptBinds_Menu : public script::IScriptBindsBase
    {
    public:
        ScriptBinds_Menu(script::IScriptSys* pSS);
        ~ScriptBinds_Menu();

        void bind(void);

    private:
        int32_t Text(script::IFunctionHandler* pH);


    };

} // namespace gui

X_NAMESPACE_END