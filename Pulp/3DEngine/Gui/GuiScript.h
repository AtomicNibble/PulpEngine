#pragma once

#ifndef X_GUI_SCRIPT_H_
#define X_GUI_SCRIPT_H_

#include "String\Lexer.h"
#include "String\XParser.h"
// #include "XWindow.h"
#include "WinVar.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XWindow;
    class XGuiScriptList;

    struct XGSWinVar
    {
        XGSWinVar()
        {
            var = nullptr;
            own = false;
        }
        XWinVar* var;
        bool own;
    };

    class XGuiScript
    {
        friend class XGuiScriptList;
        friend class XWindow;

    public:
        XGuiScript();
        ~XGuiScript();

        bool Parse(core::XParser& lex);
        void Execute(XWindow* win);
        void FixUpParms(XWindow* win);

    protected:
        typedef core::Array<XGSWinVar> ParamsArr;

        int conditionReg;
        XGuiScriptList* ifList;
        XGuiScriptList* elseList;
        ParamsArr parms;
        void (*handler)(XWindow* window, core::Array<XGSWinVar>& src);
    };

    class XGuiScriptList
    {
    public:
        XGuiScriptList();
        ~XGuiScriptList();

        void Execute(XWindow* win);
        void append(XGuiScript* gs)
        {
            list.append(gs);
        }
        void FixUpParms(XWindow* win);

    private:
        core::Array<XGuiScript*> list;
    };

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_SCRIPT_H_