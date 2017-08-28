#pragma once


#ifndef _X_GEBERICUTIL_H_
#define _X_GEBERICUTIL_H_


X_NAMESPACE_BEGIN(core)

class MemoryArenaBase;

template<class T> X_INLINE constexpr const T& Max( const T& x, const T& y ) { return ( x > y ) ? x : y; }
template<class T> X_INLINE constexpr const T& Min( const T& x, const T& y ) { return ( x < y ) ? x : y; }

template<class T> X_INLINE constexpr const T& Max( const T& x, const T& y, const T& c ) { return Max(Max(x, y), c); }
template<class T> X_INLINE constexpr const T& Min(const T& x, const T& y, const T& c ) { return Min(Min(x, y), c); }




#define X_TAG(a,b,c,d)	(uint32_t)((d << 24) | (c << 16) | (b << 8) | a)


template<typename T>
X_INLINE void fill_object(T& obj, uint8_t val)
{
	memset(&obj, val, sizeof(obj));
}


template<typename T> 
X_INLINE void zero_object(T& obj)
{
	memset(&obj, 0, sizeof(obj));
}

template<typename T> 
X_INLINE void zero_this(T* pObj)
{
	memset(pObj, 0, sizeof(*pObj));
}

template<typename T>
X_INLINE void copy_object(T& dest, T& src)
{
	memcpy(&dest, &src, sizeof(src));
}

template<typename T>
X_INLINE constexpr T X_FOURCC( char a, char b, char c, char d )
{
	static_assert(sizeof(T) == 4, "error size of fourcc must be 4");
	return (T)(a | b << 8 | c << 16 | d << 24);
}


template<class T>
X_INLINE void DeleteAndNull(T*& pVal)
{
	// I don't allow new / delete anymore.
	X_ASSERT_UNREACHABLE();
	delete pVal;
	pVal = 0;
}

template<class T>
X_INLINE void SafeRelease(T*& pVal)
{
	if (pVal) {
		pVal->release();
		pVal = nullptr;
	}
}

template<class T>
X_INLINE void SafeReleaseDX(T*& pVal)
{
	if (pVal) {
		pVal->Release();
		pVal = nullptr;
	}
}

template<class _FwdIt1, class _FwdIt2>
X_INLINE void iter_swap(_FwdIt1 _Left, _FwdIt2 _Right)
{	
	Swap(*_Left, *_Right);
}

template<class T, size_t Size>
X_INLINE void Swap(T(&_Left)[Size], T(&_Right)[Size])
{
	if (&_Left != &_Right)
	{	
		T* _First1 = _Left;
		T* _Last1 = _First1 + Size;
		T* _First2 = _Right;
		for (; _First1 != _Last1; ++_First1, ++_First2) {
			iter_swap(_First1, _First2);
		}
	}
}

template<class T>
X_INLINE void Swap(T& left, T& right)
{
	T tmp = std::move(left);
	left = std::move(right);
	right = std::move(tmp);
}

template<class T, class OtherT = T> 
inline T exchange(T& val, OtherT&& newVal)
{	
	T OldVal = std::move(val);
	val = std::forward<OtherT>(newVal);
	return OldVal;
}

template <typename T, typename T2>
inline constexpr float PercentageOf(const T& sub, const T2& of)
{
	return (static_cast<float>(sub) / static_cast<float>(of)) * 100;
}


template<class InputIt, class T>
inline T accumulate(InputIt first, InputIt last, T init)
{
	for (; first != last; ++first) {
		init = init + *first;
	}
	return init;
}

template<class InputIt, class T, class BinaryOperation>
inline T accumulate(InputIt first, InputIt last, T init, BinaryOperation op)
{
	for (; first != last; ++first) {
		init += op(*first);
	}
	return init;
}

X_NAMESPACE_END

#endif // _X_GEBERICUTIL_H_