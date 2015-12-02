#pragma once

#ifndef _X_CONSOLE_VAR_X_H_
#define _X_CONSOLE_VAR_X_H_

// #include <String\xStr.h>
#include <String\StackString.h>
#include <String\StrRef.h>

#include <Math\XColor.h>

X_NAMESPACE_BEGIN(core)

class XConsole;

inline uint32_t AlphaBit(char c)
{
	return c >= 'a' && c <= 'z' ? 1 << (c - 'z' + 31) : 0;
}

inline int TextToInt(const char* s, int nCurrent, bool bBitfield)
{
	int nValue = 0;
	if (s)
	{
		char* e;
		if (bBitfield)
		{
			// Bit manipulation.
			if (*s == '^')
				// Bit number
				nValue = 1 << strtol(++s, &e, 10);
			else
				// Full number
				nValue = strtol(s, &e, 10);

			// Check letter codes.
			for (; *e >= 'a'&& *e <= 'z'; e++)
				nValue |= AlphaBit(*e);

			if (*e == '+')
				nValue = nCurrent | nValue;
			else if (*e == '-')
				nValue = nCurrent & ~nValue;
			else if (*e == '^')
				nValue = nCurrent ^ nValue;
		}
		else
			nValue = strtol(s, &e, 10);
	}
	return nValue;
}

class CVarBase : public ICVar
{
public:
	CVarBase(XConsole* pConsole, int nFlags, const char* desc);

	virtual ~CVarBase();

	// interface ICvar 
	virtual const char* GetDesc() const X_OVERRIDE;

	virtual FlagType GetFlags(void) const X_OVERRIDE;
	virtual FlagType SetFlags(FlagType flags) X_OVERRIDE;

	virtual void Release(void) X_OVERRIDE;

	virtual void ForceSet(const char* s) X_OVERRIDE;
	virtual void SetDefault(const char* s) X_OVERRIDE;

	virtual void SetOnChangeCallback(ConsoleVarFunc pChangeFunc) X_OVERRIDE;
	virtual ConsoleVarFunc GetOnChangeCallback(void) X_OVERRIDE;

	virtual void OnModified(void);

	virtual void Reset(void) X_OVERRIDE;
	// interface ~ICvar 


protected:
	const char*			Desc_;

	FlagType			Flags_;

	ConsoleVarFunc		pChangeFunc_;
	XConsole*			pConsole_;
};


class CVarBaseConst : public CVarBase
{
public:
	X_INLINE CVarBaseConst(XConsole* pConsole, const char* Name, int Flags, const char* desc);

	X_INLINE ~CVarBaseConst() X_OVERRIDE;

	X_INLINE virtual const char* GetName(void) const X_OVERRIDE;

protected:
	const char*	Name_;
};


class CVarBaseHeap : public CVarBase
{
public:
	X_INLINE CVarBaseHeap(XConsole* pConsole, const char* Name, int Flags, const char* desc);

	X_INLINE ~CVarBaseHeap() X_OVERRIDE;

	X_INLINE virtual const char* GetName(void) const X_OVERRIDE;

protected:
	string Name_;
};

/////////////////////////////////////////////////////////////////////////

template<class T>
class CVarString : public T
{
public:
	X_INLINE CVarString(XConsole* pConsole, const char* Name, const char* Default,
		int Flags, const char* desc);

	X_INLINE ~CVarString() X_OVERRIDE;

	X_INLINE virtual int GetInteger(void) const X_OVERRIDE;
	X_INLINE virtual float GetFloat(void) const X_OVERRIDE;
	X_INLINE virtual const char* GetString(CVarBase::StrBuf& buf) X_OVERRIDE;

	X_INLINE virtual void Set(const char* s) X_OVERRIDE;
	X_INLINE virtual void Set(const float f) X_OVERRIDE;
	X_INLINE virtual void Set(const int i) X_OVERRIDE;
	X_INLINE virtual VarFlag::Enum GetType(void) X_OVERRIDE;
	X_INLINE virtual void Reset(void) X_OVERRIDE;

	X_INLINE virtual const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_OVERRIDE;

	X_INLINE virtual float GetMin(void) X_OVERRIDE;
	X_INLINE virtual float GetMax(void) X_OVERRIDE;

private:
	string String_;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVarInt : public T
{
public:
	// constructor
	X_INLINE CVarInt(XConsole* pConsole, const char* Name, const int iDefault,
		int Min, int Max, int Flags, const char* desc);

	X_INLINE ~CVarInt() X_OVERRIDE;

	X_INLINE virtual int GetInteger(void) const X_OVERRIDE;
	X_INLINE virtual float GetFloat(void) const X_OVERRIDE;
	X_INLINE virtual const char* GetString(CVarBase::StrBuf& buf) X_OVERRIDE;
	X_INLINE virtual void SetDefault(const char* s) X_OVERRIDE;

	X_INLINE virtual void Set(const char* s) X_OVERRIDE;
	X_INLINE virtual void Set(const float f) X_OVERRIDE;
	X_INLINE virtual void Set(const int i) X_OVERRIDE;
	X_INLINE virtual VarFlag::Enum GetType(void) X_OVERRIDE;
	X_INLINE virtual void Reset(void) X_OVERRIDE;

	X_INLINE virtual const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_OVERRIDE;
	X_INLINE virtual float GetMin(void) X_OVERRIDE;
	X_INLINE virtual float GetMax(void) X_OVERRIDE;

protected:
	int 		IntValue_;
	int			IntMax_;
	int			IntMin_;
	int			IntDefault_;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVarFloat : public T
{
public:
	// constructor
	CVarFloat(XConsole* pConsole, const char* Name, const float fDefault,
		float Min, float Max, int nFlags, const char* desc);

	X_INLINE ~CVarFloat() X_OVERRIDE;

	X_INLINE virtual int GetInteger(void) const X_OVERRIDE;
	X_INLINE virtual float GetFloat(void) const X_OVERRIDE;
	X_INLINE virtual const char* GetString(CVarBase::StrBuf& buf) X_OVERRIDE;
	X_INLINE virtual void SetDefault(const char* s) X_OVERRIDE;

	X_INLINE virtual void Set(const char* s) X_OVERRIDE;
	X_INLINE virtual void Set(const float f) X_OVERRIDE;
	X_INLINE virtual void Set(const int i) X_OVERRIDE;
	X_INLINE virtual VarFlag::Enum GetType(void) X_OVERRIDE;
	X_INLINE virtual void Reset(void) X_OVERRIDE;

	X_INLINE virtual const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_OVERRIDE;
	X_INLINE virtual float GetMin(void) X_OVERRIDE;
	X_INLINE virtual float GetMax(void) X_OVERRIDE;

private:
	float 		fValue_;
	float		fMax_;
	float		fMin_;
	float		fDefault_;
};

//////////////////////////////////////////////////////////////////////////
class CVarIntRef : public CVarBaseConst
{
public:
	// constructor
	X_INLINE CVarIntRef(XConsole* pConsole, const char* Name, int* pVar,
		int Min, int Max, int nFlags, const char* desc);

	X_INLINE ~CVarIntRef() X_OVERRIDE;

	X_INLINE virtual int GetInteger(void) const X_OVERRIDE;
	X_INLINE virtual float GetFloat(void) const X_OVERRIDE;
	X_INLINE virtual const char* GetString(CVarBase::StrBuf& buf) X_OVERRIDE;

	X_INLINE virtual void Set(const char* s) X_OVERRIDE;
	X_INLINE virtual void Set(const float f) X_OVERRIDE;
	X_INLINE virtual void Set(const int i) X_OVERRIDE;
	X_INLINE virtual VarFlag::Enum GetType(void) X_OVERRIDE;
	X_INLINE virtual void Reset(void) X_OVERRIDE;

	X_INLINE virtual const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_OVERRIDE;
	X_INLINE virtual float GetMin(void) X_OVERRIDE;
	X_INLINE virtual float GetMax(void) X_OVERRIDE;
protected:
	X_NO_ASSIGN(CVarIntRef);

	int& 		IntValue_;
	int			IntMax_;
	int			IntMin_;
	int			DefaultVal_;
};

//////////////////////////////////////////////////////////////////////////
class CVarFloatRef : public CVarBaseConst
{
public:
	// constructor
	X_INLINE CVarFloatRef(XConsole* pConsole, const char* Name, float* pVal,
		float Min, float Max, int nFlags, const char* desc);

	X_INLINE ~CVarFloatRef() X_OVERRIDE;

	X_INLINE virtual int GetInteger(void) const X_OVERRIDE;
	X_INLINE virtual float GetFloat(void) const X_OVERRIDE;
	X_INLINE virtual const char* GetString(CVarBase::StrBuf& buf) X_OVERRIDE;

	X_INLINE virtual void Set(const char* s) X_OVERRIDE;
	X_INLINE virtual void Set(const float f) X_OVERRIDE;
	X_INLINE virtual void Set(const int i) X_OVERRIDE;
	X_INLINE virtual VarFlag::Enum GetType(void) X_OVERRIDE;
	X_INLINE virtual void Reset(void) X_OVERRIDE;

	X_INLINE virtual const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_OVERRIDE;
	X_INLINE virtual float GetMin(void) X_OVERRIDE;
	X_INLINE virtual float GetMax(void) X_OVERRIDE;

private:
	X_NO_ASSIGN(CVarFloatRef);

	float& 		fValue_;
	float		fMax_;
	float		fMin_;
	float		fDefault_;
};


//////////////////////////////////////////////////////////////////////////
class CVarColRef : public CVarBaseConst
{
public:
	// constructor
	X_INLINE CVarColRef(XConsole* pConsole, 
		const char* Name, Color* pVal, int nFlags, const char* desc);

	X_INLINE ~CVarColRef() X_OVERRIDE;

	X_INLINE virtual int GetInteger(void) const X_OVERRIDE;
	X_INLINE virtual float GetFloat(void) const X_OVERRIDE;
	X_INLINE virtual const char* GetString(CVarBase::StrBuf& buf) X_OVERRIDE;
	X_INLINE virtual const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_OVERRIDE;

	virtual void Set(const char* s) X_OVERRIDE;
	X_INLINE virtual void Set(const float f) X_OVERRIDE;
	X_INLINE virtual void Set(const int i) X_OVERRIDE;
	X_INLINE virtual void Reset(void) X_OVERRIDE;

	X_INLINE virtual VarFlag::Enum GetType(void) X_OVERRIDE;
	X_INLINE virtual float GetMin(void) X_OVERRIDE;
	X_INLINE virtual float GetMax(void) X_OVERRIDE;
	X_INLINE const Color& GetColor(void) const;
	X_INLINE const Color& GetDefaultColor(void) const;

	static bool ColorFromString(const char* pStr, Color& out, bool Slient = true);

private:
	X_NO_ASSIGN(CVarColRef);

	Color&	ColValue_;
	Color	ColDefault_;
};


//////////////////////////////////////////////////////////////////////////
class CVarVec3Ref : public CVarBaseConst
{
public:
	// constructor
	X_INLINE CVarVec3Ref(XConsole* pConsole, const char* Name, Vec3f* pVal,
		int nFlags, const char* desc);

	X_INLINE ~CVarVec3Ref() X_OVERRIDE;

	X_INLINE virtual int GetInteger(void) const X_OVERRIDE;
	X_INLINE virtual float GetFloat(void) const X_OVERRIDE;
	X_INLINE virtual const char* GetString(CVarBase::StrBuf& buf) X_OVERRIDE;
	X_INLINE virtual const char* GetDefaultStr(CVarBase::StrBuf& buf) const X_OVERRIDE;

	virtual void Set(const char* s) X_OVERRIDE;
	X_INLINE virtual void Set(const float f) X_OVERRIDE;
	X_INLINE virtual void Set(const int i) X_OVERRIDE;

	X_INLINE virtual void Reset(void) X_OVERRIDE;

	X_INLINE virtual VarFlag::Enum GetType(void) X_OVERRIDE;
	X_INLINE virtual float GetMin(void) X_OVERRIDE;
	X_INLINE virtual float GetMax(void) X_OVERRIDE;

	X_INLINE const Vec3f& GetVal(void) const;
	X_INLINE const Vec3f& GetDefaultVal(void) const;

	static bool Vec3FromString(const char* pStr, Vec3f& out, bool Slient = true);

private:
	X_NO_ASSIGN(CVarVec3Ref);

	Vec3f&	Value_;
	Vec3f	Default_;
};

X_NAMESPACE_END

#include "ConsoleVariable.inl"

#endif // _X_CONSOLE_VAR_X_H_