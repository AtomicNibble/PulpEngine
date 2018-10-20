

template<class T>
inline Flags<T>::Flags(void) :
    flags_(0)
{
}

template<class T>
inline Flags<T>::Flags(Enum flag) :
    flags_(flag)
{
}

template<class T>
inline Flags<T>::Flags(uint32_t flags) :
    flags_(flags)
{
}

template<class T>
inline void Flags<T>::Set(Enum flag)
{
    flags_ |= flag;
}

template<class T>
inline void Flags<T>::Remove(Enum flag)
{
    flags_ &= ~flag;
}

template<class T>
inline void Flags<T>::Clear(void)
{
    flags_ = 0;
}

template<class T>
inline bool Flags<T>::IsSet(Enum flag) const
{
    return ((flags_ & flag) != 0);
}

template<class T>
inline bool Flags<T>::IsAnySet(void) const
{
    return (flags_ != 0);
}

template<class T>
inline bool Flags<T>::AreAllSet(void) const
{
    return (flags_ == ((1u << T::FLAGS_COUNT) - 1u));
}

template<class T>
inline Flags<T> Flags<T>::operator|(Flags<T> other) const
{
    return Flags<T>(flags_ | other.flags_);
}

template<class T>
inline Flags<T>& Flags<T>::operator|=(Flags<T> other)
{
    flags_ |= other.flags_;
    return *this;
}

template<class T>
inline Flags<T> Flags<T>::operator&(Flags other) const
{
    return Flags<T>(flags_ & other.flags_);
}

template<class T>
inline Flags<T>& Flags<T>::operator&=(Flags other)
{
    flags_ &= other.flags_;
    return *this;
}

template<class T>
inline Flags<T> Flags<T>::operator^(Flags other) const
{
    return Flags<T>(flags_ ^ other.flags_);
}

template<class T>
inline Flags<T>& Flags<T>::operator^=(Flags other)
{
    flags_ ^= other.flags_;
    return *this;
}

// enum only versions.
template<class T>
inline Flags<T> Flags<T>::operator|(Enum e) const
{
    return Flags<T>(flags_ | e);
}

template<class T>
inline Flags<T>& Flags<T>::operator|=(Enum e)
{
    flags_ |= e;
    return *this;
}

template<class T>
inline Flags<T> Flags<T>::operator&(Enum e) const
{
    return Flags<T>(flags_ & e);
}

template<class T>
inline Flags<T>& Flags<T>::operator&=(Enum e)
{
    flags_ &= e;
    return *this;
}

template<class T>
inline Flags<T> Flags<T>::operator^(Enum e) const
{
    return Flags<T>(flags_ ^ e);
}

template<class T>
inline Flags<T>& Flags<T>::operator^=(Enum e)
{
    flags_ ^= e;
    return *this;
}

template<class T>
inline Flags<T>& Flags<T>::operator=(Enum e)
{
    flags_ = e;
    return *this;
}

template<class T>
inline Flags<T>& Flags<T>::operator=(const Flags& oth)
{
    flags_ = oth.flags_;
    return *this;
}

template<class T>
inline Flags<T> Flags<T>::operator~(void) const
{
    // do we want to ensure bits outside setable range are not set?
    // if not we can just do ~flags_
    return Flags(~flags_ & ((1u << T::FLAGS_COUNT) - 1u));
}

template<class T>
inline uint32_t Flags<T>::ToInt(void) const
{
    return flags_;
}

template<class T>
const char* Flags<T>::ToString(Description& description) const
{
    int offset = 0;
    for (size_t i = 0; i < T::FLAGS_COUNT; ++i) {
        if ((flags_ & (1u << i)) != 0) {
            offset += _snprintf_s(description + offset, sizeof(description) - offset, _TRUNCATE, "%s, ", T::ToString(1u << i));
        }
    }

    // remove the trailing comma, if any
    if (offset > 1) {
        description[offset - 2] = 0;
    }
    else {
        description[0] = '\0';
    }

    return description;
}

template<class T>
inline bool Flags<T>::operator==(const Flags& other) const
{
    return flags_ == other.flags_;
}

template<class T>
inline bool Flags<T>::operator!=(const Flags& other) const
{
    return flags_ != other.flags_;
}

///=======================================================

template<class T>
inline Flags8<T>::Flags8(void) :
    flags_(0)
{
}

template<class T>
inline Flags8<T>::Flags8(Enum flag) :
    flags_(flag)
{
}

template<class T>
inline Flags8<T>::Flags8(uint8_t flags) :
    flags_(flags)
{
}

template<class T>
inline void Flags8<T>::Set(Enum flag)
{
    flags_ |= flag;
}

template<class T>
inline void Flags8<T>::Remove(Enum flag)
{
    flags_ &= ~flag;
}

template<class T>
inline void Flags8<T>::Clear(void)
{
    flags_ = 0;
}

template<class T>
inline bool Flags8<T>::IsSet(Enum flag) const
{
    return ((flags_ & flag) != 0);
}

template<class T>
inline bool Flags8<T>::IsAnySet(void) const
{
    return (flags_ != 0);
}

template<class T>
inline bool Flags8<T>::AreAllSet(void) const
{
    return (flags_ == ((1ull << T::FLAGS_COUNT) - 1u));
}

template<class T>
inline bool Flags8<T>::operator==(const Flags8& other) const
{
    return flags_ == other.flags_;
}

template<class T>
inline bool Flags8<T>::operator!=(const Flags8& other) const
{
    return flags_ != other.flags_;
}

template<class T>
inline Flags8<T> Flags8<T>::operator|(Flags8 other) const
{
    return Flags8<T>(flags_ | other.flags_);
}

template<class T>
inline Flags8<T>& Flags8<T>::operator|=(Flags8 other)
{
    flags_ |= other.flags_;
    return *this;
}

template<class T>
inline Flags8<T> Flags8<T>::operator&(Flags8 other) const
{
    return Flags8<T>(flags_ & other.flags_);
}

template<class T>
inline Flags8<T>& Flags8<T>::operator&=(Flags8 other)
{
    flags_ &= other.flags_;
    return *this;
}

template<class T>
inline Flags8<T> Flags8<T>::operator^(Flags8 other) const
{
    return Flags8<T>(flags_ ^ other.flags_);
}

template<class T>
inline Flags8<T>& Flags8<T>::operator^=(Flags8 other)
{
    flags_ ^= other.flags_;
    return *this;
}

// enum only versions.
template<class T>
inline Flags8<T> Flags8<T>::operator|(Enum e) const
{
    return Flags8<T>(flags_ | e);
}

template<class T>
inline Flags8<T>& Flags8<T>::operator|=(Enum e)
{
    flags_ |= e;
    return *this;
}

template<class T>
inline Flags8<T> Flags8<T>::operator&(Enum e) const
{
    return Flags8<T>(flags_ & e);
}

template<class T>
inline Flags8<T>& Flags8<T>::operator&=(Enum e)
{
    flags_ &= e;
    return *this;
}

template<class T>
inline Flags8<T> Flags8<T>::operator^(Enum e) const
{
    return Flags8<T>(flags_ ^ e);
}

template<class T>
inline Flags8<T>& Flags8<T>::operator^=(Enum e)
{
    flags_ ^= e;
    return *this;
}

template<class T>
inline Flags8<T>& Flags8<T>::operator=(Enum e)
{
    flags_ = e;
    return *this;
}

template<class T>
inline Flags8<T>& Flags8<T>::operator=(const Flags8& oth)
{
    flags_ = oth.flags_;
    return *this;
}

template<class T>
inline uint8_t Flags8<T>::ToInt(void) const
{
    return flags_;
}

template<class T>
const char* Flags8<T>::ToString(Description& description) const
{
    int offset = 0;
    for (size_t i = 0; i < T::FLAGS_COUNT; ++i) {
        if ((flags_ & (1u << i)) != 0) {
            offset += _snprintf_s(description + offset, sizeof(description) - offset, _TRUNCATE, "%s, ", T::ToString(1u << i));
        }
    }

    // remove the trailing comma, if any
    if (offset > 1) {
        description[offset - 2] = 0;
    }
    else {
        description[0] = '\0';
    }

    return description;
}
