
X_NAMESPACE_BEGIN(core)

template<typename T>
X_INLINE UniquePointerBase<T>::UniquePointerBase(core::MemoryArenaBase* arena, T* pInstance) :
	arena_(arena), 
	pInstance_(pInstance)
{

}


template<typename T>
X_INLINE core::MemoryArenaBase* UniquePointerBase<T>::getArena(void) const
{
	return arena_;
}

template<typename T>
X_INLINE typename UniquePointerBase<T>::pointer& UniquePointerBase<T>::ptr(void)
{
	return pInstance_;
}

template<typename T>
X_INLINE const typename UniquePointerBase<T>::pointer& UniquePointerBase<T>::ptr(void) const
{
	return pInstance_;
}

// ---------------------------------------

template<typename T>
X_INLINE UniquePointer<T>::UniquePointer(core::MemoryArenaBase* arena) :
	Mybase(arena, pointer())
{
	X_ASSERT_NOT_NULL(arena);
}

template<typename T>
X_INLINE UniquePointer<T>::UniquePointer(core::MemoryArenaBase* arena, nullptr_t) :
	Mybase(arena, pointer())
{
	X_ASSERT_NOT_NULL(arena);
}

template<typename T>
X_INLINE UniquePointer<T>::UniquePointer(core::MemoryArenaBase* arena, T* pInstance) :
	Mybase(arena, pInstance)
{
	X_ASSERT_NOT_NULL(pInstance);
	X_ASSERT_NOT_NULL(arena);
}

template<typename T>
X_INLINE UniquePointer<T>::UniquePointer(UniquePointer&& oth) :
	pInstance_(oth.release()),
	arena_(oth.arena_)
{
}

template<typename T>
X_INLINE UniquePointer<T>::~UniquePointer()
{
	if (get() != pointer()) {
		deleter(get());
	}
}

template<typename T>
X_INLINE UniquePointer<T>& UniquePointer<T>::operator=(nullptr_t)
{
	// assign a null pointer
	reset();
	return *this;
}


template<typename T>
X_INLINE UniquePointer<T>& UniquePointer<T>::operator=(UniquePointer&& rhs)
{
	// assign by moving _Right
	if (this != &rhs)
	{
		// different, do the move
		reset(rhs.release());
		arena_ = rhs.arena_;
	}
	return *this;
}


template<typename T>
X_INLINE T& UniquePointer<T>::operator* () const
{
	return *get();
}

template<typename T>
X_INLINE typename UniquePointer<T>::pointer UniquePointer<T>::operator-> () const
{
	return get();
}

template<typename T>
X_INLINE typename UniquePointer<T>::pointer UniquePointer<T>::get(void) const
{
	return ptr();
}

template<typename T>
X_INLINE UniquePointer<T>::operator bool() const
{	
	return (get() != pointer());
}

template<typename T>
X_INLINE typename UniquePointer<T>::pointer UniquePointer<T>::release(void)
{
	// yield ownership of pointer
	pointer pAns = get();
	ptr() = pointer();
	return pAns;
}

template<typename T>
X_INLINE void UniquePointer<T>::reset(pointer ptr_)
{
	// establish new pointer
	pointer pOld = get();
	ptr() = ptr_;
	if (pOld != pointer()) {
		deleter(pOld);
	}
}

template<typename T>
X_INLINE void UniquePointer<T>::swap(UniquePointer& oth)
{
	std::move(pInstance_, oth.pInstance_);
	std::move(arena_, oth.arena_);
}


// ------------------------------------------------------

template<typename T>
X_INLINE UniquePointer<T[]>::UniquePointer(core::MemoryArenaBase* arena) :
	Mybase(arena, pointer())
{
	X_ASSERT_NOT_NULL(arena);
}

template<typename T>
X_INLINE UniquePointer<T[]>::UniquePointer(core::MemoryArenaBase* arena, nullptr_t) :
	Mybase(arena, pointer())
{
	X_ASSERT_NOT_NULL(arena);
}

template<typename T>
X_INLINE UniquePointer<T[]>::UniquePointer(core::MemoryArenaBase* arena, T* pInstance) :
	Mybase(arena, pInstance)
{
	X_ASSERT_NOT_NULL(pInstance);
	X_ASSERT_NOT_NULL(arena);
}

template<typename T>
X_INLINE UniquePointer<T[]>::UniquePointer(UniquePointer&& oth) :
	Mybase(oth.arena_, oth.release())
{
}

template<typename T>
X_INLINE UniquePointer<T[]>::~UniquePointer()
{
	if (get() != pointer()) {
		deleter(get());
	}
}

template<typename T>
X_INLINE UniquePointer<T[]>& UniquePointer<T[]>::operator=(nullptr_t)
{
	// assign a null pointer
	reset();
	return *this;
}


template<typename T>
X_INLINE UniquePointer<T[]>& UniquePointer<T[]>::operator=(UniquePointer&& rhs)
{
	// assign by moving _Right
	if (this != &rhs)
	{
		// different, do the move
		reset(rhs.release());
		arena_ = rhs.arena_;
	}
	return *this;
}


template<typename T>
X_INLINE T& UniquePointer<T[]>::operator* () const
{
	return *get();
}

template<typename T>
X_INLINE typename UniquePointer<T[]>::pointer UniquePointer<T[]>::operator-> () const
{
	return get();
}

template<typename T>
X_INLINE typename UniquePointer<T[]>::pointer UniquePointer<T[]>::get(void) const
{
	return ptr();
}

template<typename T>
X_INLINE UniquePointer<T[]>::operator bool() const
{
	return (get() != pointer());
}

template<typename T>
X_INLINE typename UniquePointer<T[]>::pointer UniquePointer<T[]>::release(void)
{
	// yield ownership of pointer
	pointer pAns = get();
	ptr() = pointer();
	return pAns;
}

template<typename T>
X_INLINE void UniquePointer<T[]>::reset(pointer ptr_)
{
	// establish new pointer
	pointer pOld = get();
	ptr() = ptr_;
	if (pOld != pointer()) {
		deleter(pOld);
	}
}

template<typename T>
X_INLINE void UniquePointer<T[]>::swap(UniquePointer& oth)
{
	std::move(ptr(), oth.ptr());
	std::move(arena_, oth.arena_);
}


// ------------------------------------------------------

template<class _Ty>
void swap(UniquePointer<_Ty>& _Left, UniquePointer<_Ty>& _Right)
{	// swap _Left with _Right
	_Left.swap(_Right);
}

template<class _Ty1, class _Ty2>
bool operator==(const UniquePointer<_Ty1>& _Left, const UniquePointer<_Ty2>& _Right)
{	// test if UniquePointer _Left equals _Right
	return (_Left.get() == _Right.get());
}

template<class _Ty1, class _Ty2>
bool operator!=(const UniquePointer<_Ty1>& _Left, const UniquePointer<_Ty2>& _Right)
{	// test if UniquePointer _Left doesn't equal _Right
	return (!(_Left == _Right));
}

template<class _Ty1, class _Ty2>
bool operator<(const UniquePointer<_Ty1>& _Left, const UniquePointer<_Ty2>& _Right)
{	// test if UniquePointer _Left precedes _Right
	typedef typename UniquePointer<_Ty1>::pointer _Ptr1;
	typedef typename UniquePointer<_Ty2>::pointer _Ptr2;
	typedef typename common_type<_Ptr1, _Ptr2>::type _Common;

	return (less<_Common>()(_Left.get(), _Right.get()));
}

template<class _Ty1, class _Ty2>
bool operator>=(const UniquePointer<_Ty1>& _Left, const UniquePointer<_Ty2>& _Right)
{	// test if UniquePointer _Left doesn't precede _Right
	return (!(_Left < _Right));
}

template<class _Ty1, class _Ty2>
bool operator>(const UniquePointer<_Ty1>& _Left, const UniquePointer<_Ty2>& _Right)
{	// test if UniquePointer _Right precedes _Left
	return (_Right < _Left);
}

template<class _Ty1, class _Ty2>
bool operator<=(const UniquePointer<_Ty1>& _Left, const UniquePointer<_Ty2>& _Right)
{	// test if UniquePointer _Right doesn't precede _Left
	return (!(_Right < _Left));
}

template<class _Ty>
bool operator==(const UniquePointer<_Ty>& _Left, nullptr_t)
{	// test if UniquePointer == nullptr
	return (!_Left);
}

template<class _Ty>
bool operator==(nullptr_t, const UniquePointer<_Ty>& _Right)
{	// test if nullptr == UniquePointer
	return (!_Right);
}

template<class _Ty>
bool operator!=(const UniquePointer<_Ty>& _Left, nullptr_t _Right)
{	// test if UniquePointer != nullptr
	return (!(_Left == _Right));
}

template<class _Ty>
bool operator!=(nullptr_t _Left, const UniquePointer<_Ty>& _Right)
{	// test if nullptr != UniquePointer
	return (!(_Left == _Right));
}

template<class _Ty>
bool operator<(const UniquePointer<_Ty>& _Left, nullptr_t _Right)
{	// test if UniquePointer < nullptr
	typedef typename UniquePointer<_Ty>::pointer _Ptr;
	return (less<_Ptr>()(_Left.get(), _Right));
}

template<class _Ty>
bool operator<(nullptr_t _Left, const UniquePointer<_Ty>& _Right)
{	// test if nullptr < UniquePointer
	typedef typename UniquePointer<_Ty>::pointer _Ptr;
	return (less<_Ptr>()(_Left, _Right.get()));
}

template<class _Ty>
bool operator>=(const UniquePointer<_Ty>& _Left, nullptr_t _Right)
{	// test if UniquePointer >= nullptr
	return (!(_Left < _Right));
}

template<class _Ty>
bool operator>=(nullptr_t _Left, const UniquePointer<_Ty>& _Right)
{	// test if nullptr >= UniquePointer
	return (!(_Left < _Right));
}

template<class _Ty>
bool operator>(const UniquePointer<_Ty>& _Left, nullptr_t _Right)
{	// test if UniquePointer > nullptr
	return (_Right < _Left);
}

template<class _Ty>
bool operator>(nullptr_t _Left, const UniquePointer<_Ty>& _Right)
{	// test if nullptr > UniquePointer
	return (_Right < _Left);
}

template<class _Ty>
bool operator<=(const UniquePointer<_Ty>& _Left, nullptr_t _Right)
{	// test if UniquePointer <= nullptr
	return (!(_Right < _Left));
}

template<class _Ty>
bool operator<=(nullptr_t _Left, const UniquePointer<_Ty>& _Right)
{	// test if nullptr <= UniquePointer
	return (!(_Right < _Left));
}


X_NAMESPACE_END
