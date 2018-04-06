#pragma once

// place to dump user defined literals.

inline size_t operator"" _sz(unsigned long long int x)
{
    return static_cast<size_t>(x);
}

inline uint16_t operator"" _ui16(unsigned long long int x)
{
    return static_cast<uint16_t>(x);
}

inline uint32_t operator"" _ui32(unsigned long long int x)
{
    return static_cast<uint32_t>(x);
}

inline uint64_t operator"" _ui64(unsigned long long int x)
{
    return static_cast<uint64_t>(x);
}

inline int16_t operator"" _i16(unsigned long long int x)
{
    return static_cast<int16_t>(x);
}

inline int32_t operator"" _i32(unsigned long long int x)
{
    return static_cast<int32_t>(x);
}

inline int64_t operator"" _i64(unsigned long long int x)
{
    return static_cast<int64_t>(x);
}