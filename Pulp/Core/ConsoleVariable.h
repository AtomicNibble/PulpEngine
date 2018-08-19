#pragma once

#ifndef _X_CONSOLE_VAR_X_H_
#define _X_CONSOLE_VAR_X_H_

// #include <String\xStr.h>
#include <String\StackString.h>
#include <String\StrRef.h>

#include <Math\XColor.h>

X_NAMESPACE_BEGIN(core)

class XConsole;

class CVarBase : public ICVar
{
public:
    CVarBase(XConsole* pConsole, VarFlags flags, const char* pDesc);

    virtual ~CVarBase();

    // interface ICvar
    virtual const char* GetDesc(void) const X_OVERRIDE;
    virtual void SetDesc(const char* pDesc) X_OVERRIDE;

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
    const char* pDesc_;

    FlagType flags_;

    ConsoleVarFunc changeFunc_;
    XConsole* pConsole_;
};

class CVarBaseConst : public CVarBase
{
public:
    X_INLINE CVarBaseConst(XConsole* pConsole, const char* pName, VarFlags flags, const char* pDesc);

    X_INLINE ~CVarBaseConst() X_OVERRIDE;

    X_INLINE virtual const char* GetName(void) const X_OVERRIDE;

protected:
    const char* pName_;
};

class CVarBaseHeap : public CVarBase
{
public:
    X_INLINE CVarBaseHeap(XConsole* pConsole, const char* pName, VarFlags flags, const char* pDesc);

    X_INLINE ~CVarBaseHeap() X_OVERRIDE;

    X_INLINE virtual const char* GetName(void) const X_OVERRIDE;

protected:
    string name_;
};

/////////////////////////////////////////////////////////////////////////

template<class T>
class CVarString : public T
{
public:
    X_INLINE CVarString(XConsole* pConsole, const char* pName, const char* Default,
        VarFlags flags, const char* pDesc);

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
    string String_;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVarInt : public T
{
public:
    // constructor
    X_INLINE CVarInt(XConsole* pConsole, const char* pName, const int iDefault,
        int Min, int Max, VarFlags Flags, const char* pDesc);

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
    int IntValue_;
    int IntMin_;
    int IntMax_;
    int IntDefault_;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVarFloat : public T
{
public:
    // constructor
    CVarFloat(XConsole* pConsole, const char* pName, const float fDefault,
        float Min, float Max, VarFlags nFlags, const char* pDesc);

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
    X_INLINE CVarIntRef(XConsole* pConsole, const char* pName, int* pVar,
        int Min, int Max, VarFlags nFlags, const char* pDesc);

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

    int& IntValue_;
    int IntMin_;
    int IntMax_;
    int DefaultVal_;
};

//////////////////////////////////////////////////////////////////////////
class CVarFloatRef : public CVarBaseConst
{
public:
    // constructor
    X_INLINE CVarFloatRef(XConsole* pConsole, const char* pName, float* pVal,
        float Min, float Max, VarFlags nFlags, const char* pDesc);

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
        const char* pName, Color* pVal, VarFlags nFlags, const char* pDesc);

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

    Color& ColValue_;
    Color ColDefault_;
};

//////////////////////////////////////////////////////////////////////////
class CVarVec3Ref : public CVarBaseConst
{
public:
    // constructor
    X_INLINE CVarVec3Ref(XConsole* pConsole, const char* pName, Vec3f* pVal,
        VarFlags nFlags, const char* pDesc);

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

    Vec3f& Value_;
    Vec3f Default_;
};

X_NAMESPACE_END

#include "ConsoleVariable.inl"

#endif // _X_CONSOLE_VAR_X_H_