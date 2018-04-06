
inline Crc32::Crc32()
{
    buildTable();
}

inline uint32_t Crc32::GetCRC32(const char* text) const
{
    size_t len = strlen(text);
    return GetCRC32(text, len);
}

inline uint32_t Crc32::GetCRC32(const char* data, size_t size) const
{
    uint32_t crc = Begin();
    Update(data, size, crc);
    return Finish(crc);
}

inline uint32_t Crc32::GetCRC32(const uint8_t* data, size_t size) const
{
    uint32_t crc = Begin();
    Update(reinterpret_cast<const char*>(data), size, crc);
    return Finish(crc);
}

template<typename T>
uint32_t Crc32::GetCRC32OfObject(const T& obj)
{
    return GetCRC32(reinterpret_cast<const char*>(&obj), sizeof(T));
}

inline uint32_t Crc32::GetCRC32Lowercase(const char* text) const
{
    size_t len = strlen(text);
    return GetCRC32Lowercase(text, len);
}

inline uint32_t Crc32::GetCRC32Lowercase(const char* data, size_t size) const
{
    uint32_t crc = Begin();
    UpdateLowerCase(data, size, crc);
    return Finish(crc);
}

inline uint32_t Crc32::Begin(void)
{
    return CRC32_INIT_VALUE;
}

inline uint32_t Crc32::Finish(uint32_t crcvalue)
{
    return crcvalue ^ CRC32_XOR_VALUE;
}

inline uint32_t Crc32::ToLower(uint32_t c)
{
    union
    {
        uint8_t as_bytes[4];
        uint32_t as_int32;
    };

    as_int32 = c;
    as_bytes[0] = ToLower(as_bytes[0]);
    as_bytes[1] = ToLower(as_bytes[1]);
    as_bytes[2] = ToLower(as_bytes[2]);
    as_bytes[3] = ToLower(as_bytes[3]);
    return as_int32;
}

inline uint8_t Crc32::ToLower(uint8_t c)
{
    return ((((c) >= L'A') && ((c) <= L'Z')) ? ((c) - L'A' + L'a') : (c));
}

inline uint32_t Crc32::Reflect(uint32_t iReflect, const char cChar)
{
    uint32_t iValue = 0;

    // Swap bit 0 for bit 7, bit 1 For bit 6, etc....
    for (int iPos = 1; iPos < (cChar + 1); iPos++) {
        if (iReflect & 1)
            iValue |= (1 << (cChar - iPos));
        iReflect >>= 1;
    }
    return iValue;
}
