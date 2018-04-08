
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
