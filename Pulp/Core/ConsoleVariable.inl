#pragma once

X_NAMESPACE_BEGIN(core)

CVarBaseConst::CVarBaseConst(XConsole* pConsole, core::string_view name, VarFlags Flags, core::string_view desc) :
    CVarBase(pConsole, Flags, desc),
    name_(name)
{
}

CVarBaseConst::~CVarBaseConst()
{
}

core::string_view CVarBaseConst::GetName(void) const
{
    return name_;
}

// ------------------------------------------------------------

CVarBaseHeap::CVarBaseHeap(XConsole* pConsole, core::string_view name, VarFlags Flags, core::string_view desc) :
    CVarBase(pConsole, Flags, desc),
    name_(name.data(), name.length())
{
}

CVarBaseHeap::~CVarBaseHeap()
{
}

core::string_view CVarBaseHeap::GetName(void) const
{
    return core::string_view(name_);
}

// ------------------------------------------------------------

template<class T>
CVarString<T>::CVarString(XConsole* pConsole, core::string_view name, core::string_view value,
    VarFlags Flags, core::string_view desc) :
    T(pConsole, name, Flags | VarFlag::STRING, desc),
    string_(value.data(), value.length())
{
}

template<class T>
CVarString<T>::~CVarString()
{
}

template<class T>
int CVarString<T>::GetInteger(void) const
{
    return core::strUtil::StringToInt<int>(string_.begin(), string_.end());
}

template<class T>
float CVarString<T>::GetFloat(void) const
{
    return core::strUtil::StringToFloat<float>(string_.c_str());
}

template<class T>
const char* CVarString<T>::GetString(CVarBase::StrBuf& buf) const
{
    X_UNUSED(buf);
    return string_.c_str();
}

template<class T>
void CVarString<T>::ForceSet(const char* pStr)
{
    if (!pStr) {
        return;
    }

    // check if same?
    if (string_.compare(pStr)) {
        return;
    }

    string_ = pStr;
    CVarBase::OnModified();
}

template<class T>
void CVarString<T>::Set(const char* pStr)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    ForceSet(pStr);
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

template<class T>
int32_t CVarString<T>::GetDefaultInt(void) const
{
    return 0;
}

// ------------------------------------------------------------

template<class T>
CVarInt<T>::CVarInt(XConsole* pConsole, core::string_view name, const int iDefault,
    int Min, int Max, VarFlags Flags, core::string_view desc) :
    T(pConsole, name, Flags | VarFlag::INT, desc),
    intValue_(iDefault),
    intMin_(Min),
    intMax_(Max),
    intDefault_(iDefault)
{
#if X_DEBUG
    if (intMin_ <= intMax_) {
        X_ASSERT(iDefault >= Min && iDefault <= Max,
            "Error VarInt has a default value outside min/max")
        (iDefault, Min, Max);
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
    return intValue_;
}

template<class T>
float CVarInt<T>::GetFloat(void) const
{
    return static_cast<float>(intValue_);
}

template<class T>
const char* CVarInt<T>::GetString(CVarBase::StrBuf& buf) const
{
    sprintf_s(buf, "%d", intValue_);
    return buf;
}

template<class T>
void CVarInt<T>::SetDefault(const char* s)
{
    Set(s);
    intDefault_ = intValue_;
}

template<class T>
void CVarInt<T>::Set(const float f)
{
    Set(static_cast<int>(f));
}

template<class T>
void CVarInt<T>::Set(const int i)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    int iVal = i;

    // min bigger than max disables the check.
    if (intMin_ <= intMax_) {
        iVal = math<int32_t>::clamp(iVal, intMin_, intMax_);
    }

    if (iVal == intValue_) {
        return;
    }

    intValue_ = iVal;
    CVarBase::OnModified();
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
    intValue_ = intDefault_;
}

template<class T>
const char* CVarInt<T>::GetDefaultStr(CVarBase::StrBuf& buf) const
{
    sprintf_s(buf, "%d", intDefault_);
    return buf;
}

template<class T>
float CVarInt<T>::GetMin(void) const
{
    return static_cast<float>(intMin_);
}

template<class T>
float CVarInt<T>::GetMax(void) const
{
    return static_cast<float>(intMax_);
}

template<class T>
int32_t CVarInt<T>::GetMinInt(void) const
{
    return intMin_;
}

template<class T>
int32_t CVarInt<T>::GetMaxInt(void) const
{
    return intMax_;
}

template<class T>
int32_t CVarInt<T>::GetDefaultInt(void) const
{
    return intDefault_;
}

// ------------------------------------------------------------

template<class T>
CVarFloat<T>::CVarFloat(XConsole* pConsole, core::string_view name, const float fDefault,
    float Min, float Max, VarFlags nFlags, core::string_view desc) :
    T(pConsole, name, nFlags | VarFlag::FLOAT, desc),
    fValue_(fDefault),
    fMin_(Min),
    fMax_(Max),
    fDefault_(fDefault)
{
#if X_DEBUG
    if (fMin_ <= fMax_) {
        X_ASSERT(fDefault >= Min && fDefault <= Max,
            "Error VarFloat has a default value outside min/max")
        (fDefault, Min, Max);
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
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    float fValue = 0;
    if (s) {
        fValue = core::strUtil::StringToFloat<float>(s);
    }

    Set(fValue);
}

template<class T>
void CVarFloat<T>::Set(const float f)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    float fVal = f;

    // cap it sally.
    if (fMin_ <= fMax_) {
        fVal = math<float>::clamp(fVal, fMin_, fMax_);
    }

    if (fVal == fValue_) {
        return;
    }

    fValue_ = fVal;
    CVarBase::OnModified();
}

template<class T>
void CVarFloat<T>::Set(const int i)
{
    Set(static_cast<float>(i));
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

template<class T>
int32_t CVarFloat<T>::GetDefaultInt(void) const
{
    return static_cast<int32_t>(fDefault_);
}

// ------------------------------------------------------------

CVarIntRef::CVarIntRef(XConsole* pConsole, core::string_view name, int* pVar,
    int Min, int Max, VarFlags nFlags, core::string_view desc) :
    CVarBaseConst(pConsole, name, nFlags | VarFlag::INT, desc),
    intValue_(*pVar),
    intMin_(Min),
    intMax_(Max),
    defaultVal_(*pVar)
{
#if X_DEBUG
    if (intMin_ <= intMax_) {
        X_ASSERT(*pVar >= Min && *pVar <= Max,
            "Error VarInt has a default value outside min/max")
        (*pVar, Min, Max);
    }
#endif // !X_DEBUG
}

CVarIntRef::~CVarIntRef()
{
}

int CVarIntRef::GetInteger(void) const
{
    return intValue_;
}

float CVarIntRef::GetFloat(void) const
{
    return static_cast<float>(intValue_);
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
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    int iVal = i;

    // cap it sally.
    if (intMin_ <= intMax_) {
        iVal = math<int32_t>::clamp(i, intMin_, intMax_);
    }

    if (iVal == intValue_) {
        return;
    }

    intValue_ = iVal;
    CVarBase::OnModified();
}

VarFlag::Enum CVarIntRef::GetType(void) const
{
    return VarFlag::INT;
}

void CVarIntRef::Reset(void)
{
    bool changed = intValue_ != defaultVal_;

    intValue_ = defaultVal_;

    if (changed && changeFunc_) {
        changeFunc_.Invoke(this); // change callback.
    }
}

const char* CVarIntRef::GetDefaultStr(CVarBase::StrBuf& buf) const
{
    sprintf_s(buf, "%d", defaultVal_);
    return buf;
}

float CVarIntRef::GetMin(void) const
{
    return static_cast<float>(intMin_);
}

float CVarIntRef::GetMax(void) const
{
    return static_cast<float>(intMax_);
}

int32_t CVarIntRef::GetMinInt(void) const
{
    return intMin_;
}

int32_t CVarIntRef::GetMaxInt(void) const
{
    return intMax_;
}

int32_t CVarIntRef::GetDefaultInt(void) const
{
    return defaultVal_;
}

// ------------------------------------------------------------

CVarFloatRef::CVarFloatRef(XConsole* pConsole, core::string_view name, float* pVal,
    float Min, float Max, VarFlags nFlags, core::string_view desc) :
    CVarBaseConst(pConsole, name, nFlags | VarFlag::FLOAT, desc),
    fValue_(*pVal),
    fMin_(Min),
    fMax_(Max),
    fDefault_(*pVal)
{
#if X_DEBUG
    if (fMin_ <= fMax_) {
        X_ASSERT(*pVal >= Min && *pVal <= Max,
            "Error VarFloat has a default value outside min/max")
        (*pVal, Min, Max);
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
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    float fValue = 0;
    if (s) {
        fValue = core::strUtil::StringToFloat<float>(s);
    }

    Set(fValue);
}

void CVarFloatRef::Set(const float f)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    float fVal = f;

    if (fMin_ <= fMax_) {
        fVal = math<float>::clamp(fVal, fMin_, fMax_);
    }

    if (fVal == fValue_) {
        return;
    }

    fValue_ = f;
    CVarBase::OnModified();
}

void CVarFloatRef::Set(const int i)
{
    Set(static_cast<float>(i));
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

int32_t CVarFloatRef::GetDefaultInt(void) const
{
    return static_cast<int32_t>(fDefault_);
}

// ------------------------------------------------------------

CVarColRef::CVarColRef(XConsole* pConsole, core::string_view name, Color* pVal,
    VarFlags nFlags, core::string_view desc) :
    CVarBaseConst(pConsole, name, nFlags | VarFlag::FLOAT | VarFlag::COLOR, desc),
    colValue_(*pVal),
    colDefault_(*pVal)
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
    sprintf_s(buf, "%g %g %g %g", colValue_.r,
        colValue_.g, colValue_.b, colValue_.a);
    return buf;
}

const char* CVarColRef::GetDefaultStr(CVarBase::StrBuf& buf) const
{
    sprintf_s(buf, "%g %g %g %g", colDefault_.r,
        colDefault_.g, colDefault_.b, colDefault_.a);
    return buf;
}

void CVarColRef::Set(const float f)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    X_UNUSED(f);
    X_ASSERT_NOT_IMPLEMENTED();
}

void CVarColRef::Set(const int i)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }

    X_UNUSED(i);
    X_ASSERT_NOT_IMPLEMENTED();
}

void CVarColRef::Reset(void)
{
    colValue_ = colDefault_;
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

int32_t CVarColRef::GetDefaultInt(void) const
{
    return 1;
}

const Color& CVarColRef::GetColor(void) const
{
    return colValue_;
}

const Color& CVarColRef::GetDefaultColor(void) const
{
    return colDefault_;
}

// ------------------------------------------------------------

// constructor
CVarVec3Ref::CVarVec3Ref(XConsole* pConsole, core::string_view name, Vec3f* pVal,
    VarFlags nFlags, core::string_view desc) :
    CVarBaseConst(pConsole, name, nFlags | VarFlag::FLOAT | VarFlag::VECTOR, desc),
    value_(*pVal),
    default_(*pVal)
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
    sprintf_s(buf, "%g %g %g", value_.x,
        value_.y, value_.z);
    return buf;
}

const char* CVarVec3Ref::GetDefaultStr(CVarBase::StrBuf& buf) const
{
    sprintf_s(buf, "%g %g %g", default_.x,
        default_.y, default_.z);
    return buf;
}

void CVarVec3Ref::Set(const float f)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }
    X_UNUSED(f);
    X_ASSERT_NOT_IMPLEMENTED();
}

void CVarVec3Ref::Set(const int i)
{
    if (CVarBase::flags_.IsSet(VarFlag::READONLY)) {
        return;
    }
    X_UNUSED(i);
    X_ASSERT_NOT_IMPLEMENTED();
}

void CVarVec3Ref::Reset(void)
{
    value_ = default_;
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

int32_t CVarVec3Ref::GetDefaultInt(void) const
{
    return 1;
}

const Vec3f& CVarVec3Ref::GetVal(void) const
{
    return value_;
}

const Vec3f& CVarVec3Ref::GetDefaultVal(void) const
{
    return default_;
}

X_NAMESPACE_END
