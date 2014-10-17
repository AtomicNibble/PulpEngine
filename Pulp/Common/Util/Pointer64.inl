
template<typename T>
Pointer64<T>::Pointer64() : raw_(0) 
{
	static_assert(!std::is_pointer<T>::value, "Pointer64: type must not be a pointer");

}

template<typename T>
Pointer64<T>& Pointer64<T>::operator=(size_t val) 
{
	raw_ = static_cast<uint64_t>(val);
	return *this;
}

template<typename T>
Pointer64<T>& Pointer64<T>::operator = (T* p)
{
	raw_ = reinterpret_cast<uint64_t>(p);
	return *this;
}

template<typename T>
Pointer64<T>::operator T*() const
{
	return (T*)raw_;
}

template<typename T>
template<typename Type>
Type* Pointer64<T>::as() const
{
	return (Type*)raw_;
}


template<typename T>
const T* Pointer64<T>::operator[](int i) const {
	return ((T*)raw_ + i);
}

template<typename T>
T* Pointer64<T>::operator[](int i) {
	return ((T*)raw_ + i);
}