#pragma once

#ifndef _TOM_CRC32_H_
#define _TOM_CRC32_H_

X_NAMESPACE_BEGIN(core)

#define CRC32_INIT_VALUE	0xffffffffL
#define CRC32_XOR_VALUE		0xffffffffL

X_ALIGNED_SYMBOL(class Crc32, 128)
{
public:
	Crc32();

	void InitTable() {
		if (!tableInit_) 
			build_table();
	}

	uint32_t Combine(const uint32_t lhs, const uint32_t rhs) const {
		return lhs ^ rhs;
	}

	uint32_t GetCRC32(const char* text) const;
	uint32_t GetCRC32(const char* data, int size) const;
	uint32_t GetCRC32(const char* data, int size, uint32_t uCRC) const;

	uint32_t GetCRC32Lowercase(const char* text) const;
	uint32_t GetCRC32Lowercase(const char* data, int size, uint32_t uCRC) const;

private:
	void build_table();

	uint32_t get_CRC32(const char *data, int size, uint32_t uCRC) const;
	uint32_t get_CRC32Lowercase(const char *data, int size, uint32_t uCRC) const;

	static inline uint32_t FinishChecksum(uint32_t crcvalue);
	static inline uint8_t ToLower(uint8_t c);
	static inline uint32_t Reflect(uint32_t iReflect, const char cChar);

private:
	uint32_t crc32_table[0x100];
	bool tableInit_;
};

#include "crc32.inl"

X_NAMESPACE_END


#endif // _TOM_CRC32_H_