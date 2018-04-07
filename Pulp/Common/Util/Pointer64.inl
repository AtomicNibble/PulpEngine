
template<typename T>
X_INLINE  Pointer64<T>::Pointer64() :
    raw_(0)
{
    static_assert(!std::is_pointer<T>::value, "Pointer64: type must not be a pointer");
}

template<typename T>
X_INLINE Pointer64<T>& Pointer64<T>::operator=(size_t val)
{
    raw_ = static_cast<uint64_t>(val);
    return *this;
}

template<typename T>
X_INLINE Pointer64<T>& Pointer64<T>::operator=(T* p)
{
    raw_ = reinterpret_cast<uint64_t>(p);
    return *this;
}

template<typename T>
X_INLINE Pointer64<T>::operator T*() const
{
    return (T*)raw_;
}

template<typename T>
X_INLINE Pointer64<T>& Pointer64<T>::operator+=(const Pointer64<T>& oth)
{
    raw_ = (uint64_t)(((T*)raw_) + ((T*)oth.raw_));
    return *this;
}

template<>
X_INLINE Pointer64<void>& Pointer64<void>::operator+=(const Pointer64<void>& oth)
{
    raw_ = raw_ + oth.raw_;
    return *this;
}

template<typename T>
template<typename Type>
X_INLINE Type* Pointer64<T>::as(void) const
{
    return reinterpret_cast<Type*>(raw_);
}

template<typename T>
X_INLINE void* Pointer64<T>::asVoid(void) const
{
    return as<void>();
}

template<typename T>
X_INLINE const T* Pointer64<T>::operator[](size_t i) const
{
    return (as<T>() + i);
}

template<typename T>
X_INLINE T* Pointer64<T>::operator[](size_t i)
{
    return (as<T>() + i);
}