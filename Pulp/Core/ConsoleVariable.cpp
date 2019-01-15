#include "stdafx.h"

#include <String\Lexer.h>

#include "Console.h"
#include "ConsoleVariable.h"

#include <IConsole.h>


X_NAMESPACE_BEGIN(core)

template class CVarInt<CVarBaseConst>;
template class CVarInt<CVarBaseHeap>;

namespace
{
    inline int32_t TextToInt(const char* pStr, int32_t current, bool bitField)
    {
        if (!pStr) {
            return current;
        }

        const size_t strLen = core::strUtil::strlen(pStr);
        if (!strLen) {
            return current;
        }

        if (!bitField) {
            return core::strUtil::StringToInt<int32_t>(pStr, 10);
        }

        int32_t val = 0;
        const char* pEnd = nullptr;

        // Full number
        val = core::strUtil::StringToInt<int32_t>(pStr, &pEnd, 10);

        // Check letter codes.
        if (pEnd) {
            for (; *pEnd >= 'a' && *pEnd <= 'z'; pEnd++) {
                val |= core::bitUtil::AlphaBit(*pEnd);
            }

            if (*pEnd == '+') {
                val = current | val;
            }
            else if (*pEnd == '-') {
                val = current & ~val;
            }
            else if (*pEnd == '^') {
                val = current ^ val;
            }
        }

        return val;
    }

} // namespace

CVarBase::CVarBase(XConsole* pConsole, VarFlags nFlags, core::string_view desc) :
    desc_(desc),
    flags_(nFlags),
    pConsole_(pConsole)
{
}

CVarBase::~CVarBase()
{
}

void CVarBase::ForceSet(const char* pStr)
{
    X_ASSERT_NOT_IMPLEMENTED();
    X_UNUSED(pStr);
}

void CVarBase::SetDefault(const char* pStr)
{
    X_ASSERT_NOT_IMPLEMENTED();
    X_UNUSED(pStr);
}

ICVar::FlagType CVarBase::GetFlags(void) const
{
    return flags_;
}

ICVar::FlagType CVarBase::SetFlags(FlagType flags)
{
    flags_ = flags;
    return flags_;
}

void CVarBase::SetModified(void)
{
    flags_.Set(VarFlag::MODIFIED);
}

core::string_view CVarBase::GetDesc(void) const
{
    return desc_;
}

void CVarBase::SetDesc(core::string_view desc)
{
    desc_ = desc;
}

ICVar* CVarBase::SetOnChangeCallback(ConsoleVarFunc changeFunc)
{
    const bool wasSet = changeFunc_;

    changeFunc_ = changeFunc;

    if (!wasSet && flags_.IsSet(VarFlag::MODIFIED)) {
        changeFunc_.Invoke(this);
    }

    return this;
}

ConsoleVarFunc CVarBase::GetOnChangeCallback(void) const
{
    return changeFunc_;
}

void CVarBase::OnModified(void)
{
    flags_.Set(VarFlag::MODIFIED);

    if (changeFunc_) {
        changeFunc_.Invoke(this); // change callback.
    }
}

void CVarBase::Reset(void)
{
    // nothing to set here :D
}

// ========================================================

template<class T>
void CVarInt<T>::Set(const char* pStr)
{
    int32_t val = TextToInt(pStr, intValue_, CVarBase::flags_.IsSet(VarFlag::BITFIELD));

    Set(val);
}

// ========================================================

void CVarIntRef::Set(const char* pStr)
{
    int32_t val = TextToInt(pStr, intValue_, CVarBase::flags_.IsSet(VarFlag::BITFIELD));

    Set(val);
}

// ========================================================

bool CVarColRef::ColorFromString(const char* pStr, Color& out, bool silent)
{
    return Color::fromString(pStr, pStr + strlen(pStr), out, silent);
}

void CVarColRef::Set(const char* pStr)
{
    X_ASSERT_NOT_NULL(pStr);

    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    Color col;
    if (!ColorFromString(pStr, col, false)) {
        return;
    }

    // any diffrent?
    if (colValue_.compare(col, 0.001f)) {
        return;
    }

    // assign
    colValue_ = col;
    OnModified();
}

// ========================================================

bool CVarVec3Ref::Vec3FromString(const char* pStr, Vec3f& out, bool Slient)
{
    Vec3f vec;
    int i;

    core::XLexer lex(pStr, pStr + strlen(pStr));
    core::XLexToken token;

    for (i = 0; i < 3; i++) {
        if (lex.ReadToken(token)) {
            if (token.GetType() == TokenType::NUMBER) // && core::bitUtil::IsBitFlagSet(token.subtype,TT_FLOAT))
            {
                vec[i] = token.GetFloatValue();
            }
            else {
                X_ERROR_IF(!Slient, "CVar", "failed to set vec3, invalid input");
                return false;
            }
        }
        else {
            if (i == 1) {
                // we allow all 3 values to be set with 1 val.
                vec.y = vec.x;
                vec.z = vec.x;
                break;
            }
            else {
                X_ERROR_IF(!Slient, "CVar", "failed to set vec3, require either 1 or 3 real numbers");
                return false;
            }
        }
    }

    out = vec;
    return true;
}

void CVarVec3Ref::Set(const char* pStr)
{
    X_ASSERT_NOT_NULL(pStr);

    if (flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    Vec3f vec;

    if (!Vec3FromString(pStr, vec, false)) {
        return;
    }

    // any diffrent?
    if (value_.compare(vec, 0.001f)) {
        return;
    }

    // assign
    value_ = vec;
    OnModified();
}

X_NAMESPACE_END