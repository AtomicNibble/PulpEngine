#pragma once

#ifndef X_GUI_WINDOW_H_
#define X_GUI_WINDOW_H_

#include <IGui.h>
#include <ITimer.h>
#include <IInput.h>

#include <String\Lexer.h>
#include <String\XParser.h>
#include <Math\XExtrapolate.h>
#include <Math\XInterpolate.h>

#include "WinVar.h"
#include "RegExp.h"
#include "GuiScript.h"

#include "SimpleWindow.h"

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    const int MAX_EXPRESSION_REGISTERS = 4096;
    const int MAX_EXPRESSION_OPS = 4096;

    enum
    {
        WEXP_REG_TIME,
        WEXP_REG_NUM_PREDEFINED
    };

    X_DECLARE_ENUM(OpType)
    (
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        MOD,
        TABLE,
        GT,
        GE,
        LT,
        LE,
        EQ,
        NE,
        AND,
        OR,
        VAR,
        VAR_STR,
        VAR_FLOAT,
        VAR_INT,
        VAR_BOOL,
        COND);

    struct xOpt
    {
        OpType::Enum opType;
        int a, b, c, d;
    };

    struct XDrawWin
    {
        XDrawWin() :
            pWindow(nullptr),
            pSimpleWindow(nullptr)
        {
        }

        XWindow* pWindow;
        XWindowSimple* pSimpleWindow;
    };

    struct XRegEntry
    {
        const char* name;
        RegisterType::Enum type;
    };

    struct XTransitionData
    {
        XTransitionData() :
            pData(nullptr)
        {
        }

        XWinVar* pData;
        XInterpolateAccelDecelLinear<Vec4f> interp;
    };

    struct XTimeLineEvent
    {
        XTimeLineEvent() :
            pending(true)
        {
            script = X_NEW(XGuiScriptList, g_3dEngineArena, "TimeGuiScript");
        }
        ~XTimeLineEvent()
        {
            X_DELETE(script, g_3dEngineArena);
        }

        core::TimeVal time;
        XGuiScriptList* script;
        bool pending;
    };

    class XGui;
    class XWindowSimple;
    class XWindow
    {
    public:
        typedef Flags<WindowFlag> WindowFlags;
        typedef core::Array<XWindow*> Children;
        typedef Children::Iterator Childit;

        X_DECLARE_ENUM(ScriptFunction)
        (
            MOUSE_ENTER,
            MOUSE_LEAVE,
            ESC,
            ENTER,
            OPEN,
            CLOSE,
            ACTION // when you click it baby
        );

        static const char* s_ScriptNames[ScriptFunction::ENUM_COUNT];

        static XRegEntry s_RegisterVars[];
        static const int s_NumRegisterVars;

    private:
        static bool s_registerIsTemporary[MAX_EXPRESSION_REGISTERS]; // statics to assist during parsing

    public:
        XWindow(XGui* pGui);
        ~XWindow();

        void init(void);
        void clear(void);
        void reset(void); // Clear() + Init();

        // Parent
        X_INLINE void setParent(XWindow* pParent);
        X_INLINE XWindow* getParent(void);

        // Flags
        X_INLINE void setFlag(WindowFlag::Enum flag); // sets only one, intentional.
        X_INLINE void clearFlags(void);
        X_INLINE WindowFlags getFlags(void) const;

        // Children
        X_INLINE void addChild(XWindow* pChild);
        X_INLINE void removeChild(XWindow* pChild);
        X_INLINE XWindow* getChild(int index);
        X_INLINE size_t getNumChildren(void) const;
        X_INLINE uint32_t getIndexForChild(XWindow* pWindow) const;

        // Name
        X_INLINE const char* getName(void) const;

        // Drawing
        void reDraw(engine::IPrimativeContext* pDrawCon);
        void drawDebug(engine::IPrimativeContext* pDrawCon);

        // input
        bool OnInputEvent(const input::InputEvent& event);
        bool OnInputEventChar(const input::InputEvent& event);

        // Overrides
        virtual bool Parse(core::XParser& lex);
        virtual bool Parse(core::XFile* pFile);
        virtual bool WriteToFile(core::XFile* pFile);
        virtual void draw(engine::IPrimativeContext* pDrawCon, core::TimeVal time, float x, float y);
        virtual void drawBackground(engine::IPrimativeContext* pDrawCon, const Rectf& drawRect);
        virtual void activate(bool activate);
        virtual void gainFocus(void);
        virtual void loseFocus(void);
        virtual void gainCapture(void);
        virtual void loseCapture(void);
        virtual void sized(void);
        virtual void moved(void);
        virtual void mouseExit(void);
        virtual void mouseEnter(void);

        int ParseExpression(core::XParser& lex, XWinVar* var = nullptr);
        int ParseTerm(core::XParser& lex, XWinVar* var, int component);
        int ExpressionConstant(float f);

    private:
        // some none public drawing shiz.
        void drawBorder(engine::IPrimativeContext* pDrawCon, const Rectf& drawRect);

        void calcClientRect(void);

    private:
        int ExpressionTemporary(void);
        xOpt* ExpressionOp(void);

        int EmitOp(int a, int b, OpType::Enum opType, xOpt** opp = nullptr);

        int ParseEmitOp(core::XParser& lex, int a, OpType::Enum opType,
            int priority, xOpt** opp = nullptr);

        int ParseExpressionPriority(core::XParser& lex, int priority,
            XWinVar* var = nullptr, int component = 0);

        bool ParseString(core::XParser& lex, core::string& out);
        bool ParseVar(const core::XLexToken& token, core::XParser& lex);
        bool ParseRegEntry(const core::XLexToken& token, core::XParser& lex);
        bool ParseScriptFunction(const core::XLexToken& token, core::XParser& lex);
        bool ParseScript(core::XParser& lex, XGuiScriptList& list);

        void SaveExpressionParseState();
        void RestoreExpressionParseState();

        void EvaluateRegisters(float* registers);

        // called post parse.
        void SetupFromState(void);

    public:
        float EvalRegs(int test = -1, bool force = false);

        XWinVar* GetWinVarByName(const char* name);

        void FixUpParms(void);

        void StartTransition();
        void AddTransition(XWinVar* dest, Vec4f from, Vec4f to,
            int timeMs, float accelTime, float decelTime);

        void ResetTime(int timeMs);

    private:
        bool RunTimeEvents(core::TimeVal time);
        void Time(core::TimeVal time);

        void Transition(void);

        bool RunScriptList(XGuiScriptList* src);
        bool RunScript(ScriptFunction::Enum func);

    protected:
        Rectf rectDraw_;
        Rectf rectClient_;
        Rectf rectText_;

        // Registers: shit that can be changed by code.
        XWinRect rect_;
        XWinColor backColor_;
        XWinColor foreColor_;
        XWinColor hoverColor_;
        XWinColor borderColor_;
        XWinBool visable_;
        XWinBool hideCursor_;
        XWinFloat textScale_;
        XWinStr text_;
        XWinStr background_;
        // ~

        // variables, can only be set by the gui file.
        // can't change at runtime.
        WindowStyle::Enum style_;
        WindowBorderStyle::Enum borderStyle_;
        float borderSize_;
        float textAlignX_;           // x offset from aligned position
        float textAlignY_;           // y offset from aligned position.
        TextAlign::Value textAlign_; // alignment type flags, LEFT, MIDDLE, BOTTOM, etc..
        bool shadowText_;            // 'shadow'
        bool __pad[2];
        WindowFlags flags_;
        // if you change this, update file read/write for XWindow
        core::StackString<GUI_MAX_WINDOW_NAME_LEN> name_;
        // ~

        core::TimeVal lastTimeRun_;
        core::TimeVal timeLine_;

        uint32_t childId_; // if this is a child, this is it's id.

        XWindow* pParent_;
        XWindow* pFocusedChild_; // if a child window has the focus
        XWindow* pCaptureChild_; // if a child window has mouse capture
        XWindow* pOverChild_;    // if a child window has mouse capture

        font::IFont* pFont_;
        engine::Material* pBackgroundMat_;

        XGuiScriptList* scripts_[ScriptFunction::ENUM_COUNT];

        Children children_;
        core::Array<XDrawWin> drawWindows_;

        core::Array<XTimeLineEvent*> timeLineEvents_;
        core::Array<XTransitionData> transitions_;

        core::Array<xOpt> ops_;
        core::Array<float> expressionRegisters_;
        XRegisterList regList_;

        XGui* pGui_;

        bool* pSaveTemps_;

        bool hover_;
        bool init_;
        bool ___pad[2];
    };

#include "XWindow.inl"

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_WINDOW_H_