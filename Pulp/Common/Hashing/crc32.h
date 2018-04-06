#pragma once

#ifndef _TOM_CRC32_H_
#define _TOM_CRC32_H_

X_NAMESPACE_BEGIN(core)
X_DISABLE_WARNING(4324) // warning C4324: structure was padded due to __declspec(align())

X_ALIGNED_SYMBOL(class Crc32, 128)
{
    static const uint32_t CRC32_POLY_NORMAL = 0xEDB88320;
    static const uint32_t CRC32_INIT_VALUE = 0xffffffffL;
    static const uint32_t CRC32_XOR_VALUE = 0xffffffffL;

public:
    Crc32();

    static uint32_t Combine(const uint32_t lhs, const uint32_t rhs, const uint32_t rhs_length);
    static uint32_t zeroLengthCrc32(void);

    uint32_t GetCRC32(const char* text) const;
    uint32_t GetCRC32(const char* data, size_t size) const;
    uint32_t GetCRC32(const uint8_t* data, size_t size) const;

    template<typename T>
    X_INLINE uint32_t GetCRC32OfObject(const T& obj);

    // gets the crc32 as if all the text was lowercase.
    uint32_t GetCRC32Lowercase(const char* text) const;
    uint32_t GetCRC32Lowercase(const char* text, size_t len) const;

    // api for making a crc out of multiple buffers.
    static uint32_t Begin(void);
    static uint32_t Finish(uint32_t crc);
    uint32_t Update(const void* data, size_t size, uint32_t& crcvalue) const;
    uint32_t UpdateLowerCase(const char* text, size_t size, uint32_t& crcvalue) const;

private:
    void buildTable(void);

    static inline uint32_t ToLower(uint32_t c);
    static inline uint8_t ToLower(uint8_t c);
    static inline uint32_t Reflect(uint32_t iReflect, const char cChar);

private:
    uint32_t crc32_table_[8][0x100];
};

#include "crc32.inl"

X_ENABLE_WARNING(4324);
X_NAMESPACE_END

#endif // _TOM_CRC32_H_