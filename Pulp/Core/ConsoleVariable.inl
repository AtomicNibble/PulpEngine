#pragma once

X_NAMESPACE_BEGIN(core)


CVarBaseConst::CVarBaseConst(XConsole* pConsole, const char* Name, int Flags, const char* desc) :
	CVarBase(pConsole, Flags, desc), 
	Name_(Name) 
{
}

CVarBaseConst::~CVarBaseConst()
{

}

const char* CVarBaseConst::GetName(void) const 
{
	return Name_;
}


// ------------------------------------------------------------


CVarBaseHeap::CVarBaseHeap(XConsole* pConsole, const char* Name, int Flags, const char* desc) :
	CVarBase(pConsole, Flags, desc),
	Name_(Name)
{

}

CVarBaseHeap::~CVarBaseHeap()
{

}

const char* CVarBaseHeap::GetName(void) const 
{
	return Name_.c_str();
}

// ------------------------------------------------------------

template<class T>
CVarString<T>::CVarString(XConsole* pConsole, const char* Name, const char* Default,
	int Flags, const char* desc)
	: T(pConsole, Name, Flags | VarFlag::STRING, desc),
	String_(Default)
{
}

template<class T>
CVarString<T>::~CVarString()
{

}

template<class T>
int CVarString<T>::GetInteger(void) const
{ 
	return atoi(String_.c_str()); 
}

template<class T>
float CVarString<T>::GetFloat(void) const
{ 
	return core::strUtil::StringToFloat<float>(String_.c_str());
}

template<class T>
const char* CVarString<T>::GetString(CVarBase::StrBuf& buf) const
{ 
	X_UNUSED(buf);
	return String_.c_str(); 
}

template<class T>
void CVarString<T>::Set(const char* s)
{
	if (Flags_.IsSet(VarFlag::READONLY) || !s) {
		return;
	}

	OnModified();

	String_ = s;

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.
	}
}

template<class T>
void CVarString<T>::Set(const float f)
{
	StackString<32> val(f);
	Set(val.c_str());
}

template<class T>
void CVarString<T>::Set(const int i)
{
	StackString<32> val(i);
	Set(val.c_str());
}

template<class T>
VarFlag::Enum CVarString<T>::GetType(void) const
{
	return VarFlag::STRING;
}

template<class T>
void CVarString<T>::Reset(void)
{

}

template<class T>
const char* CVarString<T>::GetDefaultStr(CVarBase::StrBuf& buf) const
{
	core::zero_object(buf);
	return buf;
}

template<class T>
float CVarString<T>::GetMin(void) const
{ 
	return 0.f; 
}

template<class T>
float CVarString<T>::GetMax(void) const
{
	return 0.f; 
}

template<class T>
int32_t CVarString<T>::GetMinInt(void) const
{
	return 0;
}

template<class T>
int32_t CVarString<T>::GetMaxInt(void) const
{
	return 0;
}

// ------------------------------------------------------------

template<class T>
CVarInt<T>::CVarInt(XConsole* pConsole, const char* Name, const int iDefault, 
	int Min, int Max, int Flags, const char* desc)
	: T(pConsole, Name, Flags | VarFlag::INT, desc),
	IntValue_(iDefault),
	IntMin_(Min), 
	IntMax_(Max), 
	IntDefault_(iDefault)
{
#if X_DEBUG
	if (IntMin_ <= IntMax_)
	{
		X_ASSERT(iDefault >= Min && iDefault <= Max,
			"Error VarInt has a default value outside min/max")(iDefault, Min, Max);
	}
#endif // !X_DEBUG
}

template<class T>
CVarInt<T>::~CVarInt()
{

}

template<class T>
int CVarInt<T>::GetInteger(void) const
{
	return IntValue_; 
}

template<class T>
float CVarInt<T>::GetFloat(void) const
{
	return static_cast<float>(IntValue_);
}

template<class T>
const char* CVarInt<T>::GetString(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%d", IntValue_);
	return buf;
}

template<class T>
void CVarInt<T>::SetDefault(const char* s)
{
	Set(s);
	IntDefault_ = IntValue_;
}

template<class T>
void CVarInt<T>::Set(const float f)
{
	Set(static_cast<int>(f));
}

template<class T>
void CVarInt<T>::Set(const int i)
{
	if (i == IntValue_) {
		return;
	}
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	OnModified();

	IntValue_ = i;

	// min bigger than max disables the check.
	if (IntMin_ <= IntMax_)
	{
		if (IntValue_ < IntMin_)
			IntValue_ = IntMin_;
		else if (IntValue_ > IntMax_)
			IntValue_ = IntMax_;
	}

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}

template<class T>
VarFlag::Enum CVarInt<T>::GetType(void) const
{ 
	return VarFlag::INT;
}

template<class T>
void CVarInt<T>::Reset(void)
{
	// do i want to set modified here HUMUM
	// i don't think so.
	IntValue_ = IntDefault_;
}

template<class T>
const char* CVarInt<T>::GetDefaultStr(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%d", IntDefault_);
	return buf;
}

template<class T>
float CVarInt<T>::GetMin(void) const
{ 
	return static_cast<float>(IntMin_); 
}

template<class T>
float CVarInt<T>::GetMax(void) const
{ 
	return static_cast<float>(IntMax_);
}


template<class T>
int32_t CVarInt<T>::GetMinInt(void) const
{
	return IntMin_;
}

template<class T>
int32_t CVarInt<T>::GetMaxInt(void) const
{
	return IntMax_;
}

// ------------------------------------------------------------

template<class T>
CVarFloat<T>::CVarFloat(XConsole* pConsole, const char* Name, const float fDefault,
	float Min, float Max, int nFlags, const char* desc)
	: T(pConsole, Name, nFlags | VarFlag::FLOAT, desc),
	fValue_(fDefault), 
	fMin_(Min),
	fMax_(Max), 
	fDefault_(fDefault)
{
#if X_DEBUG
	if (fMin_ <= fMax_)
	{
		X_ASSERT(fDefault >= Min && fDefault <= Max,
			"Error VarFloat has a default value outside min/max")(fDefault, Min, Max);
	}
#endif // !X_DEBUG
}

template<class T>
CVarFloat<T>::~CVarFloat()
{

}

template<class T>
int CVarFloat<T>::GetInteger(void) const
{ 
	return static_cast<int>(fValue_);
}

template<class T>
float CVarFloat<T>::GetFloat(void) const
{ 
	return fValue_; 
}

template<class T>
const char* CVarFloat<T>::GetString(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%f", fValue_);
	return buf;
}

template<class T>
void CVarFloat<T>::SetDefault(const char* s)
{
	Set(s);
	fDefault_ = fValue_;
}

template<class T>
void CVarFloat<T>::Set(const char* s)
{
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	float fValue = 0;
	if (s) {
		fValue = core::strUtil::StringToFloat<float>(s);
	}

	// cap it before check :D
	if (fMin_ <= fMax_)
	{
		if (fValue_ < fMin_)
			fValue_ = fMin_;
		else if (fValue_ > fMax_)
			fValue_ = fMax_;
	}

	if (fValue == fValue_) {
		return;
	}

	OnModified();
	fValue_ = fValue;

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}

template<class T>
void CVarFloat<T>::Set(const float f)
{
	if (f == fValue_ || Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

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

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}

template<class T>
void CVarFloat<T>::Set(const int i)
{
	const float fVal = static_cast<float>(i);

	if (fVal == fValue_ || Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	OnModified();
	fValue_ = fVal;

	// cap it sally.
	if (fMin_ <= fMax_)
	{
		if (fValue_ < fMin_)
			fValue_ = fMin_;
		else if (fValue_ > fMax_)
			fValue_ = fMax_;
	}

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}

template<class T>
VarFlag::Enum CVarFloat<T>::GetType(void) const
{ 
	return VarFlag::FLOAT; 
}

template<class T>
void CVarFloat<T>::Reset(void)
{
	fValue_ = fDefault_;
}

template<class T>
const char* CVarFloat<T>::GetDefaultStr(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%g", fDefault_);
	return buf;
}

template<class T> 
float CVarFloat<T>::GetMin(void) const
{ 
	return fMin_; 
}

template<class T>
float CVarFloat<T>::GetMax(void) const
{ 
	return fMax_; 
}

template<class T>
int32_t CVarFloat<T>::GetMinInt(void) const
{
	return static_cast<int32_t>(fMin_);
}

template<class T>
int32_t CVarFloat<T>::GetMaxInt(void) const
{
	return static_cast<int32_t>(fMax_);
}

// ------------------------------------------------------------


CVarIntRef::CVarIntRef(XConsole* pConsole, const char* Name, int* pVar, 
	int Min, int Max, int nFlags, const char* desc)
	: CVarBaseConst(pConsole, Name, nFlags | VarFlag::INT, desc),
	IntValue_(*pVar),
	IntMin_(Min), 
	IntMax_(Max), 
	DefaultVal_(*pVar)
{
#if X_DEBUG
	if (IntMin_ <= IntMax_)
	{
		X_ASSERT(*pVar >= Min && *pVar <= Max,
			"Error VarInt has a default value outside min/max")(*pVar, Min, Max);
	}
#endif // !X_DEBUG
}

CVarIntRef::~CVarIntRef()
{

}

int CVarIntRef::GetInteger(void) const
{ 
	return IntValue_; 
}


float CVarIntRef::GetFloat(void) const
{ 
	return static_cast<float>(IntValue_); 
}


const char* CVarIntRef::GetString(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%d", GetInteger());
	return buf;
}


void CVarIntRef::Set(const float f)
{
	Set(static_cast<int>(f));
}


void CVarIntRef::Set(const int i)
{
	if (i == IntValue_) {
		return;
	} 
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

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

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}


VarFlag::Enum CVarIntRef::GetType(void) const
{
	return VarFlag::INT; 
}


void CVarIntRef::Reset(void)
{
	bool changed = IntValue_ != DefaultVal_;

	IntValue_ = DefaultVal_;

	if (changed && changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}


const char* CVarIntRef::GetDefaultStr(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%d", DefaultVal_);
	return buf;
}


float CVarIntRef::GetMin(void) const
{ 
	return static_cast<float>(IntMin_); 
}


float CVarIntRef::GetMax(void) const
{ 
	return static_cast<float>(IntMax_); 
}


int32_t CVarIntRef::GetMinInt(void) const
{
	return IntMin_;
}


int32_t CVarIntRef::GetMaxInt(void) const
{
	return IntMax_;
}


// ------------------------------------------------------------


CVarFloatRef::CVarFloatRef(XConsole* pConsole, const char* Name, float* pVal,
	float Min, float Max, int nFlags, const char* desc)
	: CVarBaseConst(pConsole, Name, nFlags | VarFlag::FLOAT, desc), 
	fValue_(*pVal),
	fMin_(Min), 
	fMax_(Max),
	fDefault_(*pVal)
{
#if X_DEBUG
	if (fMin_ <= fMax_)
	{
		X_ASSERT(*pVal >= Min && *pVal <= Max,
			"Error VarFloat has a default value outside min/max")(*pVal, Min, Max);
	}
#endif // !X_DEBUG
}

CVarFloatRef::~CVarFloatRef()
{

}

int CVarFloatRef::GetInteger(void) const
{ 
	return static_cast<int>(fValue_); 
}


float CVarFloatRef::GetFloat(void) const
{
	return fValue_;
}


const char* CVarFloatRef::GetString(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%g", fValue_);
	return buf;
}


void CVarFloatRef::Set(const char* s)
{
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	float fValue = 0;
	if (s) {
		fValue = core::strUtil::StringToFloat<float>(s);
	}

	if (fValue == fValue_) {
		return;
	}

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

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}


void CVarFloatRef::Set(const float f)
{
	if (f == fValue_ || Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	OnModified();
	fValue_ = f;

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}


void CVarFloatRef::Set(const int i)
{
	const float fVal = static_cast<float>(i);
	if (fVal == fValue_ || Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	OnModified();

	fValue_ = fVal;

	// cap it sally.
	if (fMin_ <= fMax_)
	{
		if (fValue_ < fMin_)
			fValue_ = fMin_;
		else if (fValue_ > fMax_)
			fValue_ = fMax_;
	}

	if (changeFunc_) {
		changeFunc_.Invoke(this); // change callback.	
	}
}


VarFlag::Enum CVarFloatRef::GetType(void) const
{ 
	return VarFlag::FLOAT;
}


void CVarFloatRef::Reset(void)
{
	fValue_ = fDefault_;
}


const char* CVarFloatRef::GetDefaultStr(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%g", fDefault_);
	return buf;
}


float CVarFloatRef::GetMin(void) const
{ 
	return fMin_; 
}


float CVarFloatRef::GetMax(void) const
{ 
	return fMax_; 
}

int32_t CVarFloatRef::GetMinInt(void) const
{
	return static_cast<int32_t>(fMin_);
}


int32_t CVarFloatRef::GetMaxInt(void) const
{
	return static_cast<int32_t>(fMax_);
}



// ------------------------------------------------------------


CVarColRef::CVarColRef(XConsole* pConsole, const char* Name, Color* pVal,
	int nFlags, const char* desc)
	: CVarBaseConst(pConsole, Name, nFlags | VarFlag::FLOAT | VarFlag::COLOR, desc),
	ColValue_(*pVal), 
	ColDefault_(*pVal)
{
}

CVarColRef::~CVarColRef()
{

}

int CVarColRef::GetInteger(void) const
{ 
	return static_cast<int>(0.f); 
}

float CVarColRef::GetFloat(void) const
{ 
	return 0.f; 
}

const char* CVarColRef::GetString(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%g %g %g %g", ColValue_.r,
		ColValue_.g, ColValue_.b, ColValue_.a);
	return buf;
}

const char* CVarColRef::GetDefaultStr(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%g %g %g %g", ColDefault_.r,
		ColDefault_.g, ColDefault_.b, ColDefault_.a);
	return buf;
}


void CVarColRef::Set(const float f)
{
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	X_UNUSED(f);
	X_ASSERT_NOT_IMPLEMENTED();
}

void CVarColRef::Set(const int i)
{
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}

	X_UNUSED(i);
	X_ASSERT_NOT_IMPLEMENTED();
}

void CVarColRef::Reset(void)
{
	ColValue_ = ColDefault_;
}

VarFlag::Enum CVarColRef::GetType(void) const
{ 
	return VarFlag::COLOR;
}

float CVarColRef::GetMin(void) const
{ 
	return 0.f; 
}

float CVarColRef::GetMax(void) const
{ 
	return 1.f; 
}


int32_t CVarColRef::GetMinInt(void) const
{
	return 0;
}


int32_t CVarColRef::GetMaxInt(void) const
{
	return 1;
}


const Color& CVarColRef::GetColor(void) const
{
	return ColValue_;
}

const Color& CVarColRef::GetDefaultColor(void) const
{
	return ColDefault_;
}

// ------------------------------------------------------------

// constructor
CVarVec3Ref::CVarVec3Ref(XConsole* pConsole, const char* Name, Vec3f* pVal, 
	int nFlags, const char* desc)
	: CVarBaseConst(pConsole, Name, nFlags | VarFlag::FLOAT | VarFlag::VECTOR, desc),
	Value_(*pVal), 
	Default_(*pVal)
{
}

CVarVec3Ref::~CVarVec3Ref()
{

}

int CVarVec3Ref::GetInteger(void) const
{ 
	return 0;
}

float CVarVec3Ref::GetFloat(void) const
{ 
	return 0.f; 
}

const char* CVarVec3Ref::GetString(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%g %g %g", Value_.x,
		Value_.y, Value_.z);
	return buf;
}

const char* CVarVec3Ref::GetDefaultStr(CVarBase::StrBuf& buf) const
{
	sprintf_s(buf, "%g %g %g", Default_.x,
		Default_.y, Default_.z);
	return buf;
}


void CVarVec3Ref::Set(const float f)
{
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}
	X_UNUSED(f);
	X_ASSERT_NOT_IMPLEMENTED();
}

void CVarVec3Ref::Set(const int i)
{
	if (Flags_.IsSet(VarFlag::READONLY)) {
		return;
	}
	X_UNUSED(i);
	X_ASSERT_NOT_IMPLEMENTED();
}

void CVarVec3Ref::Reset(void)
{
	Value_ = Default_;
}

VarFlag::Enum CVarVec3Ref::GetType(void) const
{ 
	return VarFlag::VECTOR; 
}

float CVarVec3Ref::GetMin(void) const
{ 
	return 0.f; 
}

float CVarVec3Ref::GetMax(void) const
{ 
	return 1.f; 
}

int32_t CVarVec3Ref::GetMinInt(void) const
{
	return 0;
}

int32_t CVarVec3Ref::GetMaxInt(void) const
{
	return 1;
}


const Vec3f& CVarVec3Ref::GetVal(void) const
{
	return Value_;
}

const Vec3f& CVarVec3Ref::GetDefaultVal(void) const
{
	return Default_;
}


X_NAMESPACE_END
