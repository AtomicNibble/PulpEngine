#include "stdafx.h"
#include "GuiScript.h"

#include "XWindow.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    namespace
    {
        void Script_Set(XWindow* window, core::Array<XGSWinVar>& src)
        {
            X_UNUSED(window);
            X_UNUSED(src);
        }

        void Script_ResetTime(XWindow* window, core::Array<XGSWinVar>& src)
        {
            if (src.isEmpty() || src[0].var->getType() != VarType::STRING)
                return;

            XWinStr* pParm = static_cast<XWinStr*>(src[0].var);

            window->ResetTime(::atoi(*pParm));
            window->EvalRegs(-1, true);
        }

        void Script_Transition(XWindow* window, core::Array<XGSWinVar>& src)
        {
            // transitions always affect rect or vec4 vars
            if (src.size() >= 4) {
                VarType::Enum type = src[0].var->getType();

                if (type == VarType::RECT) {
                    if (src[1].var->getType() != VarType::RECT || src[2].var->getType() != VarType::RECT || src[3].var->getType() != VarType::STRING) {
                        X_WARNING("Gui", "invalid transition args: %s %s %s",
                            VarType::ToString(src[1].var->getType()),
                            VarType::ToString(src[2].var->getType()),
                            VarType::ToString(src[3].var->getType()));
                        return;
                    }

                    XWinRect* from = static_cast<XWinRect*>(src[1].var);
                    XWinRect* to = static_cast<XWinRect*>(src[2].var);
                    XWinStr* timeStr = static_cast<XWinStr*>(src[3].var);

                    int time = ::atoi(*timeStr);
                    float ac = 0.0f;
                    float dc = 0.0f;

                    XWinRect* pRect = static_cast<XWinRect*>(src[0].var);
                    pRect->SetEval(false);

                    window->AddTransition(pRect, from->asVec4(), to->asVec4(), time, ac, dc);
                }
                else if (type == VarType::VEC4 || type == VarType::COLOR) {
                    if (src[1].var->getType() != VarType::VEC4 || src[2].var->getType() != VarType::VEC4 || src[3].var->getType() != VarType::STRING) {
                        X_WARNING("Gui", "invalid transition args: %s %s %s",
                            VarType::ToString(src[1].var->getType()),
                            VarType::ToString(src[2].var->getType()),
                            VarType::ToString(src[3].var->getType()));
                        return;
                    }

                    XWinVec4* from = static_cast<XWinVec4*>(src[1].var);
                    XWinVec4* to = static_cast<XWinVec4*>(src[2].var);
                    XWinStr* timeStr = static_cast<XWinStr*>(src[3].var);

                    int time = ::atoi(*timeStr);
                    float ac = 0.0f;
                    float dc = 0.0f;

                    XWinVec4* pVec4 = static_cast<XWinVec4*>(src[0].var);
                    pVec4->SetEval(false);
                    window->AddTransition(pVec4, *from, *to, time, ac, dc);
                }
                else {
                    X_WARNING("Gui", "invalid base var type for trainsition: %s",
                        VarType::ToString(type));
                    return;
                }

                window->StartTransition();
            }
        }

        struct guiCommand
        {
            const char* name;
            void (*handler)(XWindow* window, core::Array<XGSWinVar>& src);
            size_t MinParms;
            size_t MaxParms;
        };

        guiCommand g_commandList[] = {
            {"set", Script_Set, 2, 999},
            {"resetTime", Script_ResetTime, 0, 2},
            {"transition", Script_Transition, 4, 6},
        };

        static const size_t g_scriptCommandCount = sizeof(g_commandList) / sizeof(guiCommand);

    } // namespace

    XGuiScript::XGuiScript() :
        parms(g_3dEngineArena)
    {
        ifList = nullptr;
        elseList = nullptr;
        conditionReg = -1;
        handler = nullptr;
        parms.setGranularity(2);
    }

    XGuiScript::~XGuiScript()
    {
        X_DELETE(ifList, g_3dEngineArena);
        X_DELETE(elseList, g_3dEngineArena);
        size_t c = parms.size();
        for (size_t i = 0; i < c; i++) {
            if (parms[i].own) {
                X_DELETE(parms[i].var, g_3dEngineArena);
            }
        }
    }

    bool XGuiScript::Parse(core::XParser& lex)
    {
        int i;

        // first token should be function call
        // then a potentially variable set of parms
        // ended with a ;
        core::XLexToken token;
        if (!lex.ReadToken(token)) {
            return false;
        }

        core::StackString<128> name(token.begin(), token.end());
        core::StackString<256> temp(token.begin(), token.end());

        handler = nullptr;
        for (i = 0; i < g_scriptCommandCount; i++) {
            if (name.isEqual(g_commandList[i].name)) {
                handler = g_commandList[i].handler;
                break;
            }
        }

        if (handler == nullptr) {
            X_ERROR("Gui", "unknown function '%s'", name.c_str());
            return false;
        }

        // now read parms til ;
        // all parms are read as idWinStr's but will be fixed up later
        // to be proper types
        X_DISABLE_WARNING(4127)
        while (true)
            X_ENABLE_WARNING(4127)
            {
                if (!lex.ReadToken(token)) {
                    X_ERROR("Gui", "unexpected end of file, while parsing '%s'", name.c_str());
                    return false;
                }

                temp.set(token.begin(), token.end());

                if (temp.isEqual(";")) {
                    break;
                }

                if (temp.isEqual("}")) {
                    lex.UnreadToken(token);
                    break;
                }

                XWinStr* str = X_NEW(XWinStr, g_3dEngineArena, "TransVar");
                *str = core::string(temp.c_str());
                XGSWinVar wv;
                wv.own = true;
                wv.var = str;
                parms.append(wv);
            }

        //  verify min/max params
        if (parms.size() < g_commandList[i].MinParms || parms.size() > g_commandList[i].MaxParms) {
            X_ERROR("Gui", "incorrect number of parms for function '%s' min:%i max:%i provided:%i", name.c_str(),
                g_commandList[i].MinParms, g_commandList[i].MaxParms, parms.size());
            return false;
        }

        return true;
    }

    void XGuiScript::Execute(XWindow* win)
    {
        if (handler) {
            handler(win, parms);
        }
    }

    void XGuiScript::FixUpParms(XWindow* win)
    {
        // tickle my pickle, for a nickle?
        if (handler == &Script_Transition) {
            if (parms[0].var->getType() == VarType::STRING) {
                XWinStr* pStr = static_cast<XWinStr*>(parms[0].var);
                XWinVar* pDest = win->GetWinVarByName(*pStr);

                bool isRect = pDest == win->GetWinVarByName("rect");

                if (pDest) {
                    X_DELETE(parms[0].var, g_3dEngineArena);
                    parms[0].var = pDest;
                    parms[0].own = false;
                }
                else {
                    X_WARNING("Gui", "Window '%s' transition does not have a valid destination var '%s'",
                        win->getName(), pStr->c_str());
                    return;
                }

                int i;
                for (i = 1; i < 3; i++) {
                    if (parms[i].var->getType() != VarType::STRING) {
                        continue;
                    }

                    pStr = static_cast<XWinStr*>(parms[i].var);

                    if (isRect) {
                        XWinRect* rect = X_NEW(XWinRect, g_3dEngineArena, "TransParam");
                        rect->Set(pStr->c_str());
                        parms[i].var = rect;
                    }
                    else {
                        XWinVec4* v4 = X_NEW(XWinVec4, g_3dEngineArena, "TransParam");
                        v4->Set(pStr->c_str());
                        parms[i].var = v4;
                    }

                    parms[i].own = true;

                    X_DELETE(pStr, g_3dEngineArena);
                }
            }
        }
        else {
            size_t i, num = parms.size();
            for (i = 0; i < num; i++) {
                parms[i].var->Init(parms[i].var->c_str(), win);
            }
        }
    }

    // ====================== XGuiScriptList =======================

    XGuiScriptList::XGuiScriptList() :
        list(g_3dEngineArena)
    {
        list.setGranularity(4);
    }

    XGuiScriptList::~XGuiScriptList()
    {
    }

    void XGuiScriptList::Execute(XWindow* win)
    {
        size_t i, num = list.size();

        for (i = 0; i < num; i++) {
            XGuiScript* pGs = list[i];
            X_ASSERT_NOT_NULL(pGs);

            if (pGs->conditionReg >= 0) {
                /*if (win->HasOps())
				{
					float f = win->EvalRegs(gs->conditionReg);
					if (f) {
						if (gs->ifList) {
							win->RunScriptList(gs->ifList);
						}
					}
					else if (gs->elseList) {
						win->RunScriptList(gs->elseList);
					}
				}*/
            }
            pGs->Execute(win);
        }
    }

    void XGuiScriptList::FixUpParms(XWindow* win)
    {
        size_t i, num = list.size();

        for (i = 0; i < num; i++) {
            XGuiScript* pGs = list[i];
            pGs->FixUpParms(win);

            if (pGs->ifList) {
                pGs->ifList->FixUpParms(win);
            }
            if (pGs->elseList) {
                pGs->elseList->FixUpParms(win);
            }
        }
    }

} // namespace gui

X_NAMESPACE_END