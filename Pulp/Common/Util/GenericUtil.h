#pragma once


#ifndef _X_GEBERICUTIL_H_
#define _X_GEBERICUTIL_H_


X_NAMESPACE_BEGIN(core)

class MemoryArenaBase;

template<class T> X_INLINE T Max( T x, T y ) { return ( x > y ) ? x : y; }
template<class T> X_INLINE T Min( T x, T y ) { return ( x < y ) ? x : y; }

template<class T> X_INLINE T Max( T x, T y, T c ) { return Max(Max(x, y), c); }
template<class T> X_INLINE T Min( T x, T y, T c ) { return Min(Min(x, y), c); }


template <uint32_t val1, uint32_t val2 >
struct Max_static_unsigned
{
	static const uint32_t value = ((val1 < val2) ? val2 : val1);
};

template <int32_t val1, int32_t val2 >
struct Max_static_signed
{
	static const int32_t value = ((val1 < val2) ? val2 : val1);
};

template <size_t val1, size_t val2 >
struct Max_static_size
{
	static const size_t value = ((val1 < val2) ? val2 : val1);
};



#define X_TAG(a,b,c,d)	(uint32_t)((d << 24) | (c << 16) | (b << 8) | a);




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
X_INLINE T X_FOURCC( char a, char b, char c, char d )
{
	static_assert(sizeof(T) == 4, "error size of fourcc must be 4");
	return (T)(a | b << 8 | c << 16 | d << 24);
}

X_INLINE int PadInt( int val, int pad )
{
	int mod = ( val % pad );

	if( mod != 0 )
		val = val + ( pad - mod );

	return val;
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



template<class T>
X_INLINE void Swap(T& left, T& right)
{
	T tmp = left;
	left = right; 
	right = tmp;
}

template<class T, class OtherT = T> 
inline T exchange(T& val, OtherT&& newVal)
{	
	T OldVal = std::move(val);
	val = std::forward<OtherT>(newVal);
	return OldVal;
}

template <typename T, typename T2>
inline float PercentageOf(const T& sub, const T2& of)
{
	return (static_cast<float>(sub) / static_cast<float>(of)) * 100;
}


X_NAMESPACE_END

#endif // _X_GEBERICUTIL_H_