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
    inline Flags(Enum flag);
    inline Flags(const Flags<T>& oth);
    inline explicit Flags(uint32_t flags);

    inline void Set(Enum flag);
    inline void Remove(Enum flag);
    inline void Clear(void);

    inline bool IsSet(Enum flag) const;
    inline bool IsAnySet(void) const;
    inline bool AreAllSet(void) const;

    // compare
    inline bool operator==(const Flags& other) const;
    inline bool operator!=(const Flags& other) const;

    // Bitwise operators.
    inline Flags operator|(Flags other) const;
    inline Flags& operator|=(Flags other);
    inline Flags operator&(Flags other) const;
    inline Flags& operator&=(Flags other);
    inline Flags operator^(Flags other) const;
    inline Flags& operator^=(Flags other);

    // enum only versions.
    inline Flags operator|(Enum e) const;
    inline Flags& operator|=(Enum e);
    inline Flags operator&(Enum e) const;
    inline Flags& operator&=(Enum e);
    inline Flags operator^(Enum e) const;
    inline Flags& operator^=(Enum e);

    inline Flags& operator=(Enum e);
    inline Flags& operator=(const Flags& oth);

    inline Flags operator~(void)const;

    // Returns the flags' value as integer.
    inline uint32_t ToInt(void) const;

    /// human-readable string, and returns a pointer to the description string.
    const char* ToString(Description& description) const;

private:
    union
    {
        uint32_t flags_;
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
    inline Flags8(void);
    inline Flags8(Enum flag);
    inline Flags8(const Flags8<T>& oth);
    inline explicit Flags8(uint8_t flags);

    inline void Set(Enum flag);

    inline void Remove(Enum flag);
    inline void Clear(void);

    inline bool IsSet(Enum flag) const;
    inline bool IsAnySet(void) const;
    inline bool AreAllSet(void) const;

    // compare
    inline bool operator==(const Flags8& other) const;
    inline bool operator!=(const Flags8& other) const;

    inline Flags8 operator|(Flags8 other) const;
    inline Flags8& operator|=(Flags8 other);
    inline Flags8 operator&(Flags8 other) const;
    inline Flags8& operator&=(Flags8 other);
    inline Flags8 operator^(Flags8 other) const;
    inline Flags8& operator^=(Flags8 other);

    // enum only versions.
    inline Flags8 operator|(Enum e) const;
    inline Flags8& operator|=(Enum e);
    inline Flags8 operator&(Enum e) const;
    inline Flags8& operator&=(Enum e);
    inline Flags8 operator^(Enum e) const;
    inline Flags8& operator^=(Enum e);

    inline Flags8& operator=(Enum e);
    inline Flags8& operator=(const Flags8& oth);

    inline uint8_t ToInt(void) const;

    /// human-readable string, and returns a pointer to the description string.
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
