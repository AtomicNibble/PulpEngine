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

TELEM_INLINE void* AlignTop(void* ptr, tt_size alignment)
{
    union
    {
        void* as_void;
        tt_uintptr as_uintptr_t;
    };

    const tt_size mask = alignment - 1;
    as_void = ptr;
    as_uintptr_t += mask;
    as_uintptr_t &= ~mask;
    return as_void;
}

template<typename T>
TELEM_INLINE constexpr bool IsAligned(T value, unsigned int alignment, unsigned int offset)
{
    return ((value + offset) % alignment) == 0;
}

TELEM_INLINE bool Convert(const wchar_t* pSrc, tt_int32 srcSize,
    char* output, tt_int32 outputBytes, tt_int32& lengthOut)
{
    lengthOut = 0;
    if (!outputBytes) {
        return false;
    }

    const tt_int32 bytesWritten = WideCharToMultiByte(
        CP_UTF8,
        0,
        pSrc,
        srcSize,
        output,
        outputBytes,
        nullptr,
        nullptr
    );

    if (bytesWritten == 0) {
        // error
        return false;
    }

    lengthOut = bytesWritten;
    return true;
}
