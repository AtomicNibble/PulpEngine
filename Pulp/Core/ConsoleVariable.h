#pragma once

#ifndef _X_CONSOLE_VAR_X_H_
#define _X_CONSOLE_VAR_X_H_

#include <String\StackString.h>
#include <String\StrRef.h>

#include <Math\XColor.h>

X_NAMESPACE_BEGIN(core)

class XConsole;

class CVarBase : public ICVar
{
public:
    CVarBase(XConsole* pConsole, VarFlags flags, core::string_view desc);

    virtual ~CVarBase();

    // interface ICvar
    virtual core::string_view GetDesc(void) const X_OVERRIDE;
    virtual void SetDesc(core::string_view desc) X_OVERRIDE;

    virtual FlagType GetFlags(void) const X_OVERRIDE;
    virtual FlagType SetFlags(FlagType flags) X_OVERRIDE;
    virtual void SetModified(void) X_FINAL;

    virtual void ForceSet(const char* s) X_OVERRIDE;
    virtual void SetDefault(const char* s) X_OVERRIDE;

    virtual ICVar* SetOnChangeCallback(ConsoleVarFunc pChangeFunc) X_OVERRIDE;
    virtual ConsoleVarFunc GetOnChangeCallback(void) const X_OVERRIDE;

    virtual void OnModified(void);

    virtual void Reset(void) X_OVERRIDE;
    // interface ~ICvar

protected:
    core::string_view desc_;

    FlagType flags_;

    ConsoleVarFunc changeFunc_;
    XConsole* pConsole_;
};

class CVarBaseConst : public CVarBase
{
public:
    X_INLINE CVarBaseConst(XConsole* pConsole, core::string_view name, VarFlags flags, core::string_view desc);

    X_INLINE ~CVarBaseConst() X_OVERRIDE;

    X_INLINE virtual core::string_view GetName(void) const X_OVERRIDE;

protected:
    core::string_view name_;
};

class CVarBaseHeap : public CVarBase
{
public:
    X_INLINE CVarBaseHeap(XConsole* pConsole, core::string_view name, VarFlags flags, core::string_view desc);

    X_INLINE ~CVarBaseHeap() X_OVERRIDE;

    X_INLINE virtual core::string_view GetName(void) const X_OVERRIDE;

protected:
    string name_;
};

/////////////////////////////////////////////////////////////////////////

template<class T>
class CVarString : public T
{
public:
    X_INLINE CVarString(XConsole* pConsole, core::string_view name, core::string_view value,
        VarFlags flags, core::string_view desc);

    X_INLINE ~CVarString() X_FINAL;

    X_INLINE int GetInteger(void) const X_FINAL;
    X_INLINE float GetFloat(void) const X_FINAL;
    X_INLINE const char* GetString(CVarBase::StrBuf& buf) const X_FINAL;

    X_INLINE void ForceSet(const char* s) X_FINAL;
    X_INLINE void Set(const char* s) X_FINAL;
    X_INLINE void Set(const float f) X_FINAL;
    X_INLINE void Set(const int i) X_FINAL;
    X_INLINE VarFlag::Enum GetType(void) const X_FINAL;
    X_INLINE void Reset(void) X_FINAL;

    X_INLINE const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_FINAL;

    X_INLINE float GetMin(void) const X_FINAL;
    X_INLINE float GetMax(void) const X_FINAL;
    X_INLINE int32_t GetMinInt(void) const X_FINAL;
    X_INLINE int32_t GetMaxInt(void) const X_FINAL;
    X_INLINE int32_t GetDefaultInt(void) const X_FINAL;

private:
    string string_;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVarInt : public T
{
public:
    // constructor
    X_INLINE CVarInt(XConsole* pConsole, core::string_view name, const int iDefault,
        int Min, int Max, VarFlags Flags, core::string_view desc);

    X_INLINE ~CVarInt() X_FINAL;

    X_INLINE int GetInteger(void) const X_FINAL;
    X_INLINE float GetFloat(void) const X_FINAL;
    X_INLINE const char* GetString(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE void SetDefault(const char* s) X_FINAL;

    void Set(const char* s) X_FINAL;
    X_INLINE void Set(const float f) X_FINAL;
    X_INLINE void Set(const int i) X_FINAL;
    X_INLINE VarFlag::Enum GetType(void) const X_FINAL;
    X_INLINE void Reset(void) X_FINAL;

    X_INLINE const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE float GetMin(void) const X_FINAL;
    X_INLINE float GetMax(void) const X_FINAL;
    X_INLINE int32_t GetMinInt(void) const X_FINAL;
    X_INLINE int32_t GetMaxInt(void) const X_FINAL;
    X_INLINE int32_t GetDefaultInt(void) const X_FINAL;

protected:
    int intValue_;
    int intMin_;
    int intMax_;
    int intDefault_;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVarFloat : public T
{
public:
    // constructor
    CVarFloat(XConsole* pConsole, core::string_view name, const float fDefault,
        float Min, float Max, VarFlags nFlags, core::string_view desc);

    X_INLINE ~CVarFloat() X_FINAL;

    X_INLINE int GetInteger(void) const X_FINAL;
    X_INLINE float GetFloat(void) const X_FINAL;
    X_INLINE const char* GetString(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE void SetDefault(const char* s) X_FINAL;

    X_INLINE void Set(const char* s) X_FINAL;
    X_INLINE void Set(const float f) X_FINAL;
    X_INLINE void Set(const int i) X_FINAL;
    X_INLINE VarFlag::Enum GetType(void) const X_FINAL;
    X_INLINE void Reset(void) X_FINAL;

    X_INLINE const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE float GetMin(void) const X_FINAL;
    X_INLINE float GetMax(void) const X_FINAL;
    X_INLINE int32_t GetMinInt(void) const X_FINAL;
    X_INLINE int32_t GetMaxInt(void) const X_FINAL;
    X_INLINE int32_t GetDefaultInt(void) const X_FINAL;

private:
    float fValue_;
    float fMin_;
    float fMax_;
    float fDefault_;
};

//////////////////////////////////////////////////////////////////////////
class CVarIntRef : public CVarBaseConst
{
public:
    // constructor
    X_INLINE CVarIntRef(XConsole* pConsole, core::string_view name, int* pVar,
        int Min, int Max, VarFlags nFlags, core::string_view desc);

    X_INLINE ~CVarIntRef() X_FINAL;

    X_INLINE int GetInteger(void) const X_FINAL;
    X_INLINE float GetFloat(void) const X_FINAL;
    X_INLINE const char* GetString(CVarBase::StrBuf& buf) const X_FINAL;

    void Set(const char* s) X_FINAL;
    X_INLINE void Set(const float f) X_FINAL;
    X_INLINE void Set(const int i) X_FINAL;
    X_INLINE VarFlag::Enum GetType(void) const X_FINAL;
    X_INLINE void Reset(void) X_FINAL;

    X_INLINE const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE float GetMin(void) const X_FINAL;
    X_INLINE float GetMax(void) const X_FINAL;
    X_INLINE int32_t GetMinInt(void) const X_FINAL;
    X_INLINE int32_t GetMaxInt(void) const X_FINAL;
    X_INLINE int32_t GetDefaultInt(void) const X_FINAL;

protected:
    X_NO_ASSIGN(CVarIntRef);

    int& intValue_;
    int intMin_;
    int intMax_;
    int defaultVal_;
};

//////////////////////////////////////////////////////////////////////////
class CVarFloatRef : public CVarBaseConst
{
public:
    // constructor
    X_INLINE CVarFloatRef(XConsole* pConsole, core::string_view name, float* pVal,
        float Min, float Max, VarFlags nFlags, core::string_view desc);

    X_INLINE ~CVarFloatRef() X_FINAL;

    X_INLINE int GetInteger(void) const X_FINAL;
    X_INLINE float GetFloat(void) const X_FINAL;
    X_INLINE const char* GetString(CVarBase::StrBuf& buf) const X_FINAL;

    X_INLINE void Set(const char* s) X_FINAL;
    X_INLINE void Set(const float f) X_FINAL;
    X_INLINE void Set(const int i) X_FINAL;
    X_INLINE VarFlag::Enum GetType(void) const X_FINAL;
    X_INLINE void Reset(void) X_FINAL;

    X_INLINE const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE float GetMin(void) const X_FINAL;
    X_INLINE float GetMax(void) const X_FINAL;
    X_INLINE int32_t GetMinInt(void) const X_FINAL;
    X_INLINE int32_t GetMaxInt(void) const X_FINAL;
    X_INLINE int32_t GetDefaultInt(void) const X_FINAL;

private:
    X_NO_ASSIGN(CVarFloatRef);

    float& fValue_;
    float fMin_;
    float fMax_;
    float fDefault_;
};

//////////////////////////////////////////////////////////////////////////
class CVarColRef : public CVarBaseConst
{
public:
    // constructor
    X_INLINE CVarColRef(XConsole* pConsole,
        core::string_view name, Color* pVal, VarFlags nFlags, core::string_view desc);

    X_INLINE ~CVarColRef() X_FINAL;

    X_INLINE int GetInteger(void) const X_FINAL;
    X_INLINE float GetFloat(void) const X_FINAL;
    X_INLINE const char* GetString(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_FINAL;

    void Set(const char* s) X_FINAL;
    X_INLINE void Set(const float f) X_FINAL;
    X_INLINE void Set(const int i) X_FINAL;
    X_INLINE void Reset(void) X_FINAL;

    X_INLINE VarFlag::Enum GetType(void) const X_FINAL;
    X_INLINE float GetMin(void) const X_FINAL;
    X_INLINE float GetMax(void) const X_FINAL;
    X_INLINE int32_t GetMinInt(void) const X_FINAL;
    X_INLINE int32_t GetMaxInt(void) const X_FINAL;
    X_INLINE int32_t GetDefaultInt(void) const X_FINAL;

    X_INLINE const Color& GetColor(void) const;
    X_INLINE const Color& GetDefaultColor(void) const;

    static bool ColorFromString(const char* pStr, Color& out, bool Slient = true);

private:
    X_NO_ASSIGN(CVarColRef);

    Color& colValue_;
    Color colDefault_;
};

//////////////////////////////////////////////////////////////////////////
class CVarVec3Ref : public CVarBaseConst
{
public:
    // constructor
    X_INLINE CVarVec3Ref(XConsole* pConsole, core::string_view name, Vec3f* pVal,
        VarFlags nFlags, core::string_view desc);

    X_INLINE ~CVarVec3Ref() X_FINAL;

    X_INLINE int GetInteger(void) const X_FINAL;
    X_INLINE float GetFloat(void) const X_FINAL;
    X_INLINE const char* GetString(CVarBase::StrBuf& buf) const X_FINAL;
    X_INLINE const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_FINAL;

    void Set(const char* s) X_FINAL;
    X_INLINE void Set(const float f) X_FINAL;
    X_INLINE void Set(const int i) X_FINAL;

    X_INLINE void Reset(void) X_FINAL;

    X_INLINE VarFlag::Enum GetType(void) const X_FINAL;
    X_INLINE float GetMin(void) const X_FINAL;
    X_INLINE float GetMax(void) const X_FINAL;
    X_INLINE int32_t GetMinInt(void) const X_FINAL;
    X_INLINE int32_t GetMaxInt(void) const X_FINAL;
    X_INLINE int32_t GetDefaultInt(void) const X_FINAL;

    X_INLINE const Vec3f& GetVal(void) const;
    X_INLINE const Vec3f& GetDefaultVal(void) const;

    static bool Vec3FromString(const char* pStr, Vec3f& out, bool Slient = true);

private:
    X_NO_ASSIGN(CVarVec3Ref);

    Vec3f& value_;
    Vec3f default_;
};

X_NAMESPACE_END

#include "ConsoleVariable.inl"

#endif // _X_CONSOLE_VAR_X_H_