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
	// virtual const char* GetName() const;
	virtual const char* GetDesc() const;

	virtual FlagType GetFlags() const;
	virtual FlagType SetFlags(FlagType flags);

	virtual void Release();

	virtual void ForceSet(const char* s);
	virtual void SetDefault(const char* s);

	virtual void SetOnChangeCallback(ConsoleVarFunc pChangeFunc);
	virtual ConsoleVarFunc GetOnChangeCallback();

	virtual void OnModified();

	virtual void Reset();

protected:
	// const char*					Name_;
	const char*					Desc_;

	FlagType					Flags_;

	ConsoleVarFunc				pChangeFunc_;
	XConsole*					pConsole_;
};


class CVarBaseConst : public CVarBase
{
public:
	CVarBaseConst(XConsole* pConsole, const char* Name, int Flags, const char* desc) :
		CVarBase(pConsole, Flags, desc), Name_(Name) {}

	virtual const char* GetName() const {
		return Name_;
	}
protected:
	const char*			Name_;
};


class CVarBaseHeap : public CVarBase
{
public:
	CVarBaseHeap(XConsole* pConsole, const char* Name, int Flags, const char* desc) :
		CVarBase(pConsole, Flags, desc), Name_(Name) {}

	virtual const char* GetName() const {
		return Name_.c_str();
	}
protected:
	string		Name_;
};

/////////////////////////////////////////////////////////////////////////

template<class T>
class CVarString : public T
{
public:
	CVarString(XConsole* pConsole, const char* Name, const char* Default, int Flags, const char* desc)
		: T(pConsole, Name, Flags | VarFlag::STRING, desc)
	{
		String_ = Default;
	}

	virtual int GetInteger() const { return atoi(String_.c_str()); }
	virtual float GetFloat() const { return (float)atof(String_.c_str()); }
	virtual const char *GetString() { return String_.c_str(); }
	virtual void Set(const char* s)
	{
		if (Flags_.IsSet(VarFlag::READONLY) || !s)
			return;

		OnModified();

		String_ = s;

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.
	}

	virtual void Set(const float f)
	{
		StackString<32> val(f);
		Set(val.c_str());
	}

	virtual void Set(const int i)
	{
		StackString<32> val(i);
		Set(val.c_str());
	}

	virtual VarFlag::Enum GetType()
	{
		return VarFlag::STRING;
	}

	virtual void Reset() {

	}

	virtual const char* GetDefaultStr() const X_OVERRIDE{
		return "";
	}

	virtual float GetMin(void) X_OVERRIDE{ return 0.f; }
	virtual float GetMax(void) X_OVERRIDE{ return 0.f; }

private:
	string String_;
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVarInt : public T
{
public:
	// constructor
	CVarInt(XConsole* pConsole, const char *Name, const int iDefault, int Min, int Max, int Flags, const char* desc)
		: T(pConsole, Name, Flags | VarFlag::INT, desc),
		IntValue_(iDefault), IntMin_(Min), IntMax_(Max), IntDefault_(iDefault)
	{
	}

	virtual int GetInteger() const { return IntValue_; }
	virtual float GetFloat() const { return static_cast<float>(IntValue_); }
	virtual const char *GetString()
	{
		static char szReturnString[64];

		sprintf(szReturnString, "%d", IntValue_);
		return szReturnString;
	}
	virtual void SetDefault(const char* s)
	{
		Set(s);
		IntDefault_ = IntValue_;
	}
	virtual void Set(const char* s)
	{
		int nValue = TextToInt(s, IntValue_, Flags_.IsSet(VarFlag::BITFIELD));

		Set(nValue);
	}
	virtual void Set(const float f)
	{
		Set((int)f);
	}
	virtual void Set(const int i)
	{
		if (i == IntValue_)
			return;
		if (Flags_.IsSet(VarFlag::READONLY))
			return;

		OnModified();

		// min bigger than max disables the check.
		if (IntMin_ <= IntMax_)
		{
			if (i < IntMin_)
				(int)i = IntMin_;
			else if (i > IntMax_)
				(int)i = IntMax_;
		}

		IntValue_ = i;

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	
	}
	virtual VarFlag::Enum GetType() { return VarFlag::INT; }

	virtual void Reset() {
		// do i want to set modified here HUMUM
		// i don't think so.
		IntValue_ = IntDefault_;
	}

	virtual const char* GetDefaultStr() const X_OVERRIDE{
		static char szReturnString[64];
		sprintf(szReturnString, "%d", IntDefault_);
		return szReturnString;
	}


	virtual float GetMin(void) X_OVERRIDE{ return static_cast<float>(IntMin_); }
	virtual float GetMax(void) X_OVERRIDE{ return static_cast<float>(IntMax_); }

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
	CVarFloat(XConsole* pConsole, const char* Name, const float fDefault, float Min, float Max, int nFlags, const char* desc)
		: T(pConsole, Name, nFlags | VarFlag::FLOAT, desc),
		fValue_(fDefault), fMin_(Min), fMax_(Max), fDefault_(fDefault)
	{
	}

	virtual int GetInteger() const { return static_cast<int>(fValue_); }
	virtual float GetFloat() const { return fValue_; }
	virtual const char *GetString()
	{
		static char szReturnString[128];

		sprintf(szReturnString, "%f", fValue_);
		return szReturnString;
	}
	virtual void SetDefault(const char* s)
	{
		Set(s);
		fDefault_ = fValue_;
	}
	virtual void Set(const char* s)
	{
		if (Flags_.IsSet(VarFlag::READONLY))
			return;

		float fValue = 0;
		if (s)
			fValue = (float)atof(s);

		// cap it before check :D
		if (fMin_ <= fMax_)
		{
			if (fValue_ < fMin_)
				fValue_ = fMin_;
			else if (fValue_ > fMax_)
				fValue_ = fMax_;
		}

		if (fValue == fValue_)
			return;

		OnModified();
		fValue_ = fValue;

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	
	}

	virtual void Set(const float f)
	{
		if (f == fValue_ || Flags_.IsSet(VarFlag::READONLY))
			return;

		OnModified();
		fValue_ = f;

		// cap it sally.
		if (fMin_ <= fMax_)
		{
			if (fValue_ < fMin_)
				fValue_ = fMin_;
			else if (fValue_ > fMax_)
				fValue_ = fMax_;
		}

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	
	}

	virtual void Set(const int i)
	{
		if ((float)i == fValue_ || Flags_.IsSet(VarFlag::READONLY))
			return;

		OnModified();
		fValue_ = (float)i;

		// cap it sally.
		if (fMin_ <= fMax_)
		{
			if (fValue_ < fMin_)
				fValue_ = fMin_;
			else if (fValue_ > fMax_)
				fValue_ = fMax_;
		}

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	

	}
	virtual VarFlag::Enum GetType() { return VarFlag::FLOAT; }

	virtual void Reset() {
		fValue_ = fDefault_;
	}

	virtual const char* GetDefaultStr() const X_OVERRIDE{
		static char szReturnString[64];
		sprintf(szReturnString, "%g", fDefault_);
		return szReturnString;
	}

	virtual float GetMin(void) X_OVERRIDE{ return fMin_; }
	virtual float GetMax(void) X_OVERRIDE{ return fMax_; }

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
	CVarIntRef(XConsole* pConsole, const char *Name, int* pVar, int Min, int Max, int nFlags, const char* desc)
		: CVarBaseConst(pConsole, Name, nFlags | VarFlag::INT, desc),
		IntValue_(*pVar), IntMin_(Min), IntMax_(Max), DefaultVal_(*pVar)
	{
	}

	virtual int GetInteger() const { return IntValue_; }
	virtual float GetFloat() const { return static_cast<float>(IntValue_); }
	virtual const char *GetString()
	{
		static char szReturnString[64];

		sprintf(szReturnString, "%d", GetInteger());
		return szReturnString;
	}
	virtual void Set(const char* s)
	{
		int nValue = TextToInt(s, IntValue_, Flags_.IsSet(VarFlag::BITFIELD));

		Set(nValue);
	}
	virtual void Set(const float f)
	{
		Set((int)f);
	}
	virtual void Set(const int i)
	{
		if (i == IntValue_)
			return;
		if (Flags_.IsSet(VarFlag::READONLY))
			return;

		OnModified();

		// cap it sally.
		IntValue_ = i;

		if (IntMin_ <= IntMax_)
		{
			if (IntValue_ < IntMin_)
				IntValue_ = IntMin_;
			else if (IntValue_ > IntMax_)
				IntValue_ = IntMax_;
		}

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	
	}
	virtual VarFlag::Enum GetType() { return VarFlag::INT; }

	virtual void Reset() {

		bool changed = IntValue_ != DefaultVal_;

		IntValue_ = DefaultVal_;

		if (changed && pChangeFunc_)
			pChangeFunc_(this); // change callback.	
	}

	virtual const char* GetDefaultStr() const X_OVERRIDE{
		static char szReturnString[64];
		sprintf(szReturnString, "%d", DefaultVal_);
		return szReturnString;
	}

	virtual float GetMin(void) X_OVERRIDE{ return static_cast<float>(IntMin_); }
	virtual float GetMax(void) X_OVERRIDE{ return static_cast<float>(IntMax_); }

protected:
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
	CVarFloatRef(XConsole* pConsole, const char* Name, float* pVal, float Min, float Max, int nFlags, const char* desc)
		: CVarBaseConst(pConsole, Name, nFlags | VarFlag::FLOAT, desc), fValue_(*pVal),
		fMin_(Min), fMax_(Max), fDefault_(*pVal)
	{
	}

	virtual int GetInteger() const { return static_cast<int>(fValue_); }
	virtual float GetFloat() const { return fValue_; }
	virtual const char *GetString()
	{
		static char szReturnString[128];

		sprintf(szReturnString, "%g", fValue_);
		return szReturnString;
	}

	virtual void Set(const char* s)
	{
		if (Flags_.IsSet(VarFlag::READONLY))
			return;

		float fValue = 0;
		if (s)
			fValue = (float)atof(s);

		if (fValue == fValue_)
			return;

		OnModified();

		// cap it sally.
		if (fMin_ <= fMax_)
		{
			if (fValue_ < fMin_)
				fValue_ = fMin_;
			else if (fValue_ > fMax_)
				fValue_ = fMax_;
		}

		fValue_ = fValue;

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	
	}

	virtual void Set(const float f)
	{
		if (f == fValue_ || Flags_.IsSet(VarFlag::READONLY))
			return;

		OnModified();
		fValue_ = f;

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	
	}

	virtual void Set(const int i)
	{
		if ((float)i == fValue_ || Flags_.IsSet(VarFlag::READONLY))
			return;

		OnModified();

		fValue_ = (float)i;

		// cap it sally.
		if (fMin_ <= fMax_)
		{
			if (fValue_ < fMin_)
				fValue_ = fMin_;
			else if (fValue_ > fMax_)
				fValue_ = fMax_;
		}

		if (pChangeFunc_)
			pChangeFunc_(this); // change callback.	

	}
	virtual VarFlag::Enum GetType() { return VarFlag::FLOAT; }

	virtual void Reset() {
		fValue_ = fDefault_;
	}

	virtual const char* GetDefaultStr() const X_OVERRIDE{
		static char szReturnString[64];
		sprintf(szReturnString, "%g", fDefault_);
		return szReturnString;
	}

	virtual float GetMin(void) X_OVERRIDE{ return fMin_; }
	virtual float GetMax(void) X_OVERRIDE{ return fMax_; }


private:
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
	CVarColRef(XConsole* pConsole, const char* Name, Color* pVal, int nFlags, const char* desc)
		: CVarBaseConst(pConsole, Name, nFlags | VarFlag::FLOAT | VarFlag::COLOR, desc),
		ColValue_(*pVal), ColDefault_(*pVal)
	{
	}

	virtual int GetInteger() const X_OVERRIDE { return static_cast<int>(0.f); }
	virtual float GetFloat() const X_OVERRIDE { return 0.f; }
	virtual const char *GetString() X_OVERRIDE
	{
		static char szReturnString[128];
		sprintf(szReturnString, "%g %g %g %g", ColValue_.r,
			ColValue_.g, ColValue_.b, ColValue_.a);
		return szReturnString;
	}
	virtual const char* GetDefaultStr() const X_OVERRIDE
	{
		static char szReturnString[64];
		sprintf(szReturnString, "%g %g %g %g", ColDefault_.r,
			ColDefault_.g, ColDefault_.b, ColDefault_.a);
		return szReturnString;
	}

	virtual void Set(const char* s) X_OVERRIDE;

	virtual void Set(const float f)
	{
		if (Flags_.IsSet(VarFlag::READONLY))
			return;

		X_UNUSED(f);
		X_ASSERT_NOT_IMPLEMENTED();
	}

	virtual void Set(const int i)
	{
		if (Flags_.IsSet(VarFlag::READONLY))
			return;

		X_UNUSED(i);
		X_ASSERT_NOT_IMPLEMENTED();
	}

	virtual void Reset() X_OVERRIDE {
		ColValue_ = ColDefault_;
	}

	virtual VarFlag::Enum GetType() X_OVERRIDE{ return VarFlag::COLOR; }
	virtual float GetMin(void) X_OVERRIDE{ return 0.f; }
	virtual float GetMax(void) X_OVERRIDE{ return 1.f; }

	const Color& GetColor(void) const {
		return ColValue_;
	}
	const Color& GetDefaultColor(void) const {
		return ColDefault_;
	}

	static bool ColorFromString(const char* pStr, Color& out, bool Slient = true);

private:
	Color&	ColValue_;
	Color	ColDefault_;
};


//////////////////////////////////////////////////////////////////////////
class CVarVec3Ref : public CVarBaseConst
{
public:
	// constructor
	CVarVec3Ref(XConsole* pConsole, const char* Name, Vec3f* pVal, int nFlags, const char* desc)
		: CVarBaseConst(pConsole, Name, nFlags | VarFlag::FLOAT | VarFlag::VECTOR, desc),
		Value_(*pVal), Default_(*pVal)
	{
	}

	virtual int GetInteger() const X_OVERRIDE{ return static_cast<int>(0.f); }
	virtual float GetFloat() const X_OVERRIDE{ return 0.f; }
	virtual const char *GetString() X_OVERRIDE
	{
		static char szReturnString[128];
		sprintf(szReturnString, "%g %g %g", Value_.x,
			Value_.y, Value_.z);
		return szReturnString;
	}
	virtual const char* GetDefaultStr() const X_OVERRIDE
	{
		static char szReturnString[64];
		sprintf(szReturnString, "%g %g %g", Default_.x,
			Default_.y, Default_.z);
		return szReturnString;
	}

	virtual void Set(const char* s) X_OVERRIDE;

	virtual void Set(const float f)
	{
		if (Flags_.IsSet(VarFlag::READONLY))
			return;
		X_UNUSED(f);
		X_ASSERT_NOT_IMPLEMENTED();
	}

	virtual void Set(const int i)
	{
		if (Flags_.IsSet(VarFlag::READONLY))
			return;
		X_UNUSED(i);
		X_ASSERT_NOT_IMPLEMENTED();
	}

	virtual void Reset() X_OVERRIDE{
		Value_ = Default_;
	}

	virtual VarFlag::Enum GetType() X_OVERRIDE{ return VarFlag::VECTOR; }
	virtual float GetMin(void) X_OVERRIDE{ return 0.f; }
	virtual float GetMax(void) X_OVERRIDE{ return 1.f; }

	const Vec3f& GetVal(void) const {
		return Value_;
	}
	const Vec3f& GetDefaultVal(void) const {
		return Default_;
	}

	static bool Vec3FromString(const char* pStr, Vec3f& out, bool Slient = true);

private:
	Vec3f&	Value_;
	Vec3f	Default_;
};

X_NAMESPACE_END

#endif // _X_CONSOLE_VAR_X_H_