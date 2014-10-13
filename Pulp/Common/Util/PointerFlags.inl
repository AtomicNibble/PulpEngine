

template <class T, size_t BIT_COUNT>
X_INLINE PointerFlags<T, BIT_COUNT>::PointerFlags(void)
	: m_pointer(nullptr)
{
}

template <class T, size_t BIT_COUNT>
X_INLINE PointerFlags<T, BIT_COUNT>::PointerFlags(T* ptr)
	: m_pointer(ptr)
{
	X_ASSERT((union_cast<uintptr_t>(ptr) & BIT_MASK) == 0, "Unable to store requesed bits in pointer flags")(ptr, BIT_COUNT, BIT_MASK);
}


template <class T, uintptr_t BIT_COUNT>
X_INLINE PointerFlags<T, BIT_COUNT>::PointerFlags(T* ptr, uintptr_t bits)
	: m_pointer(union_cast<T*>(union_cast<uintptr_t>(ptr) | bits))
{
	X_ASSERT((union_cast<uintptr_t>(ptr) & BIT_MASK) == 0, "Unable to store requesed bits in pointer flags")(ptr, BIT_COUNT, BIT_MASK);
	X_ASSERT((bits & ~BIT_MASK) == 0, "Inncorrect bits provided for pointer flags")(bits, BIT_COUNT, BIT_MASK);
}


template <class T, uintptr_t BIT_COUNT>
X_INLINE PointerFlags<T, BIT_COUNT>::PointerFlags(const PointerFlags<T, BIT_COUNT>& other)
	: m_pointer(other.m_pointer)
{
}

template <class T, uintptr_t BIT_COUNT>
X_INLINE void PointerFlags<T, BIT_COUNT>::CopyPointer(const T* ptr)
{
	const uintptr_t bits = GetBits();
	m_pointer = union_cast<T*>(union_cast<uintptr_t>(ptr) | bits);
}


template <class T, uintptr_t BIT_COUNT>
X_INLINE void PointerFlags<T, BIT_COUNT>::CopyPointer(const PointerFlags<T, BIT_COUNT>& rhs)
{
	CopyPointer(rhs.GetRawPointer());
}

template <class T, uintptr_t BIT_COUNT>
X_INLINE uintptr_t PointerFlags<T, BIT_COUNT>::GetBits(void) const
{
	return union_cast<uintptr_t>(m_pointer) & BIT_MASK;
}


template <class T, uintptr_t BIT_COUNT>
X_INLINE void PointerFlags<T, BIT_COUNT>::SetBits(uintptr_t bits)
{
	X_ASSERT((bits & ~BIT_MASK) == 0, "Unable to store requesed bits in pointer flags")(bits, BIT_COUNT, BIT_MASK);
	m_pointer = union_cast<T*>((union_cast<uintptr_t>(m_pointer) & ~BIT_MASK) | bits);
}


template <class T, uintptr_t BIT_COUNT>
template <uintptr_t BIT>
X_INLINE bool PointerFlags<T, BIT_COUNT>::IsBitSet(void) const
{
	const uintptr_t bits = GetBits();
	return ((bits & (1 << BIT)) != 0);
}


template <class T, uintptr_t BIT_COUNT>
template <uintptr_t BIT>
X_INLINE void PointerFlags<T, BIT_COUNT>::SetBit(void)
{
	m_pointer = union_cast<T*>(union_cast<uintptr_t>(m_pointer) | (1 << BIT));
}

template <class T, uintptr_t BIT_COUNT>
template <uintptr_t BIT>
X_INLINE void PointerFlags<T, BIT_COUNT>::ClearBit(void)
{
	m_pointer = union_cast<T*>(union_cast<uintptr_t>(m_pointer) & ~(1 << BIT));
}

template <class T, uintptr_t BIT_COUNT>
X_INLINE T& PointerFlags<T, BIT_COUNT>::operator*(void)
{
	return *(GetRawPointer());
}

template <class T, uintptr_t BIT_COUNT>
X_INLINE const T& PointerFlags<T, BIT_COUNT>::operator*(void) const
{
	return *(GetRawPointer());
}

template <class T, uintptr_t BIT_COUNT>
X_INLINE T* PointerFlags<T, BIT_COUNT>::operator->(void)
{
	return GetRawPointer();
}

template <class T, uintptr_t BIT_COUNT>
X_INLINE const T* PointerFlags<T, BIT_COUNT>::operator->(void) const
{
	return GetRawPointer();
}


template <class T, uintptr_t BIT_COUNT>
X_INLINE PointerFlags<T, BIT_COUNT>::operator T*(void)
{
	return GetRawPointer();
}


template <class T, uintptr_t BIT_COUNT>
X_INLINE PointerFlags<T, BIT_COUNT>::operator const T*(void) const
{
	return GetRawPointer();
}


template <class T, uintptr_t BIT_COUNT>
X_INLINE T* PointerFlags<T, BIT_COUNT>::GetRawPointer(void)
{
	return union_cast<T*>(union_cast<uintptr_t>(m_pointer) & ~BIT_MASK);
}

template <class T, uintptr_t BIT_COUNT>
X_INLINE const T* PointerFlags<T, BIT_COUNT>::GetRawPointer(void) const
{
	return union_cast<const T*>(union_cast<uintptr_t>(m_pointer) & ~BIT_MASK);
}
