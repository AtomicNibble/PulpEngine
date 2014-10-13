
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