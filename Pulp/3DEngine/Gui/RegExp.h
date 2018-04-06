#pragma once

#ifndef X_GUI_REGEXP_H_
#define X_GUI_REGEXP_H_

#include <Containers\HashMap.h>
#include <Containers\HashIndex.h>

#include <String\StringHash.h>
#include <String\Lexer.h>
#include <String\XParser.h>

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XWindow;
    class XWinVar;

    X_DECLARE_ENUM(RegisterType)
    (
        BOOL,
        INT,
        FLOAT,
        VEC2,
        VEC3,
        VEC4,
        COLOR,
        RECT,
        STRING);

    class XRegister
    {
    public:
        XRegister();
        XRegister(const char* p, RegisterType::Enum type);

        void SetToRegs(float* registers);
        void GetFromRegs(float* registers);
        void CopyRegs(XRegister* src);
        void Enable(bool b)
        {
            enabled_ = b;
        }

        static int REGCOUNT[RegisterType::ENUM_COUNT];

        bool enabled_;
        RegisterType::Enum type_;
        core::string name_;
        int regCount_;
        Vec4<uint16_t> regs_;
        XWinVar* var_;
    };

    X_INLINE XRegister::XRegister()
    {
    }

    X_INLINE XRegister::XRegister(const char* p, RegisterType::Enum type)
    {
        name_ = p;
        type_ = type;
        regCount_ = REGCOUNT[type];
        enabled_ = (type_ == RegisterType::STRING) ? false : true;
        var_ = nullptr;
    };

    X_INLINE void XRegister::CopyRegs(XRegister* src)
    {
        regs_ = src->regs_;
    }

    class XRegisterList
    {
    public:
        XRegisterList();
        ~XRegisterList();

        void AddReg(const char* name, RegisterType::Enum type, Vec4f data, XWindow* win, XWinVar* var);
        void AddReg(const char* name, RegisterType::Enum type, core::XParser& lex, XWindow* win, XWinVar* var);

        XRegister* FindReg(const char* name);
        void SetToRegs(float* registers);
        void GetFromRegs(float* registers);
        void Reset();

    private:
        //	typedef core::HashMap<core::string, XRegister*> RegMap;
        //	RegMap hash_;

        core::Array<XRegister*> registers_;
        core::XHashIndex regHash_;
    };

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_REGEXP_H_