
inline Crc32::Crc32()
{
	tableInit_ = false;
	InitTable();
}

inline uint32_t Crc32::GetCRC32(const char *text) const
{
	int len = (int)strlen(text);
	return GetCRC32(text, len, CRC32_INIT_VALUE);
}

inline uint32_t Crc32::GetCRC32(const char *data, int size) const
{
	return get_CRC32(data, size, CRC32_INIT_VALUE);
}

inline uint32_t Crc32::GetCRC32(const char *data, int size, uint32_t uCRC) const
{
	return get_CRC32(data, size, uCRC);
}


inline uint32_t Crc32::GetCRC32Lowercase(const char *text) const
{
	int len = (int)strlen(text);
	return GetCRC32Lowercase(text, len, CRC32_INIT_VALUE);
}

inline uint32_t Crc32::GetCRC32Lowercase(const char *data, int size, uint32_t uCRC) const
{
	return get_CRC32Lowercase(data, size, uCRC);
}

inline uint32_t Crc32::get_CRC32Lowercase(const char *data, int size, uint32_t crcvalue) const
{
	int32_t len;
	uint8_t* buffer;

	// Get the length. 
	len = size;
	buffer = (uint8_t*)data;

	while (len--)
	{
		uint8_t c = *buffer++;
		crcvalue = (crcvalue >> 8) ^ crc32_table[(crcvalue & 0xFF) ^ ToLower(c)];
	}

	return FinishChecksum(crcvalue);
}

inline uint32_t Crc32::get_CRC32(const char *data, int size, uint32_t crcvalue) const
{
	int len;
	uint8_t* buffer;
	uint32_t crc = crcvalue;

	len = size;
	buffer = (uint8_t*)data;

	while (len--)
		crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ *buffer++];

	return FinishChecksum(crc);
}


inline uint32_t Crc32::FinishChecksum(uint32_t crcvalue)  {
	return crcvalue ^ CRC32_XOR_VALUE;
}

inline uint8_t Crc32::ToLower(uint8_t c) {
	return ((((c) >= L'A') && ((c) <= L'Z')) ? ((c)-L'A' + L'a') : (c));
}

inline uint32_t Crc32::Reflect(uint32_t iReflect, const char cChar)
{
	uint32_t iValue = 0;

	// Swap bit 0 for bit 7, bit 1 For bit 6, etc....
	for (int iPos = 1; iPos < (cChar + 1); iPos++)
	{
		if (iReflect & 1)
			iValue |= (1 << (cChar - iPos));
		iReflect >>= 1;
	}
	return iValue;
}

