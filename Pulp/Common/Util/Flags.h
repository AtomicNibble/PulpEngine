#pragma once

#ifndef X_FLAGS_H_
#define X_FLAGS_H_

// X_NAMESPACE_BEGIN(core)

template<class T>
class Flags : public T
{
public:
    typedef typename T::Enum Enum;
    typedef typename T::Bits Bits;

    typedef char Description[512];

public:
    inline Flags(void);
    inline constexpr Flags(Enum flag);
    inline constexpr Flags(const Flags<T>& oth) = default;
    inline constexpr explicit Flags(uint32_t flags);

    inline constexpr void Set(Enum flag);
    inline constexpr void Remove(Enum flag);
    inline constexpr void Clear(void);

    inline constexpr bool IsSet(Enum flag) const;
    inline constexpr bool IsAnySet(void) const;
    inline constexpr bool AreAllSet(void) const;

    // compare
    inline constexpr bool operator==(const Flags& other) const;
    inline constexpr bool operator!=(const Flags& other) const;

    // Bitwise operators.
    inline constexpr Flags operator|(Flags other) const;
    inline constexpr Flags& operator|=(Flags other);
    inline constexpr Flags operator&(Flags other) const;
    inline constexpr Flags& operator&=(Flags other);
    inline constexpr Flags operator^(Flags other) const;
    inline constexpr Flags& operator^=(Flags other);

    // enum only versions.
    inline constexpr Flags operator|(Enum e) const;
    inline constexpr Flags& operator|=(Enum e);
    inline constexpr Flags operator&(Enum e) const;
    inline constexpr Flags& operator&=(Enum e);
    inline constexpr Flags operator^(Enum e) const;
    inline constexpr Flags& operator^=(Enum e);

    inline constexpr Flags& operator=(Enum e);
    inline constexpr Flags& operator=(const Flags& oth) = default;

    inline constexpr Flags operator~(void) const;

    // Returns the flags' value as integer.
    inline constexpr uint32_t ToInt(void) const;

    // human-readable string, and returns a pointer to the description string.
    const char* ToString(Description& description) const;

private:
    union
    {
        uint32_t flags_;
        Bits bits_;
    };
};

template<class T>
class Flags64 : public T
{
public:
    typedef typename T::Enum Enum;
    typedef typename T::Bits Bits;

    typedef char Description[512];

public:
    inline Flags64(void);
    inline constexpr Flags64(Enum flag);
    inline constexpr Flags64(const Flags64<T>& oth) = default;
    inline constexpr explicit Flags64(uint64_t flags);

    inline constexpr void Set(Enum flag);
    inline constexpr void Remove(Enum flag);
    inline constexpr void Clear(void);

    inline constexpr bool IsSet(Enum flag) const;
    inline constexpr bool IsAnySet(void) const;
    inline constexpr bool AreAllSet(void) const;

    // compare
    inline constexpr bool operator==(const Flags64& other) const;
    inline constexpr bool operator!=(const Flags64& other) const;

    // Bitwise operators.
    inline constexpr Flags64 operator|(Flags64 other) const;
    inline constexpr Flags64& operator|=(Flags64 other);
    inline constexpr Flags64 operator&(Flags64 other) const;
    inline constexpr Flags64& operator&=(Flags64 other);
    inline constexpr Flags64 operator^(Flags64 other) const;
    inline constexpr Flags64& operator^=(Flags64 other);

    // enum only versions.
    inline constexpr Flags64 operator|(Enum e) const;
    inline constexpr Flags64& operator|=(Enum e);
    inline constexpr Flags64 operator&(Enum e) const;
    inline constexpr Flags64& operator&=(Enum e);
    inline constexpr Flags64 operator^(Enum e) const;
    inline constexpr Flags64& operator^=(Enum e);

    inline constexpr Flags64& operator=(Enum e);
    inline constexpr Flags64& operator=(const Flags64& oth) = default;

    inline constexpr Flags64 operator~(void) const;

    // Returns the flags' value as integer.
    inline constexpr uint64_t ToInt(void) const;

    // human-readable string, and returns a pointer to the description string.
    const char* ToString(Description& description) const;

private:
    union
    {
        uint64_t flags_;
        Bits bits_;
    };
};

template<class T>
class Flags8 : public T
{
public:
    static const int32_t FLAG_COUNT = T::FLAGS_COUNT;
    typedef typename T::Enum Enum;
    typedef typename T::Bits Bits;

    static_assert(FLAG_COUNT <= 8, "Flags8 constructed with a flag type containing more than 8 flags");

    typedef char Description[512];

public:
    inline constexpr Flags8(void);
    inline constexpr Flags8(Enum flag);
    inline constexpr Flags8(const Flags8<T>& oth) = default;
    inline constexpr explicit Flags8(uint8_t flags);

    inline constexpr void Set(Enum flag);

    inline constexpr void Remove(Enum flag);
    inline constexpr void Clear(void);

    inline constexpr bool IsSet(Enum flag) const;
    inline constexpr bool IsAnySet(void) const;
    inline constexpr bool AreAllSet(void) const;

    // compare
    inline constexpr bool operator==(const Flags8& other) const;
    inline constexpr bool operator!=(const Flags8& other) const;

    inline constexpr Flags8 operator|(Flags8 other) const;
    inline constexpr Flags8& operator|=(Flags8 other);
    inline constexpr Flags8 operator&(Flags8 other) const;
    inline constexpr Flags8& operator&=(Flags8 other);
    inline constexpr Flags8 operator^(Flags8 other) const;
    inline constexpr Flags8& operator^=(Flags8 other);

    // enum only versions.
    inline constexpr Flags8 operator|(Enum e) const;
    inline constexpr Flags8& operator|=(Enum e);
    inline constexpr Flags8 operator&(Enum e) const;
    inline constexpr Flags8& operator&=(Enum e);
    inline constexpr Flags8 operator^(Enum e) const;
    inline constexpr Flags8& operator^=(Enum e);

    inline constexpr Flags8& operator=(Enum e);
    inline constexpr Flags8& operator=(const Flags8& oth) = default;

    inline constexpr uint8_t ToInt(void) const;

    // human-readable string, and returns a pointer to the description string.
    const char* ToString(Description& description) const;

private:
    union
    {
        uint8_t flags_;
        Bits bits_;
    };
};

#include "Flags.inl"

// X_NAMESPACE_END

#endif // X_FLAGS_H_
