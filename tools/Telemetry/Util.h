#pragma once


template<typename T>
TELEM_INLINE void zero_object(T& obj)
{
    memset(&obj, 0, sizeof(obj));
}

template<typename T>
TELEM_INLINE void zero_this(T* pObj)
{
    memset(pObj, 0, sizeof(*pObj));
}

template<typename T>
TELEM_INLINE constexpr T X_FOURCC(char a, char b, char c, char d)
{
    static_assert(sizeof(T) == 4, "error size of fourcc must be 4");
    return (T)(a | b << 8 | c << 16 | d << 24);
}

template<typename T>
TELEM_INLINE constexpr T RoundUpToMultiple(T numToRound, T multipleOf)
{
    return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
}

template<typename T>
TELEM_INLINE constexpr T RoundDownToMultiple(T numToRound, T multipleOf)
{
    return numToRound & ~(multipleOf - 1);
}


