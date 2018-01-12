#pragma once



#ifndef _X_GUID_I_H_
#define _X_GUID_I_H_

#include <Platform\Types.h>

struct EngineGUID
{
	EngineGUID();
	EngineGUID(uint32_t id, uint16_t id1, uint16_t id2,
		uint8_t byte_1, uint8_t byte_2, uint8_t byte_3, uint8_t byte_4, uint8_t byte_5, uint8_t byte_6, uint8_t byte_7, uint8_t byte_8);

	inline bool operator==(const EngineGUID& rhs) const;
	inline bool operator!=(const EngineGUID& rhs) const;
	inline bool operator<(const EngineGUID& rhs) const;


	uint64_t hi;
	uint64_t low;
};


inline EngineGUID::EngineGUID() :
	hi(0),
	low(0)
{

}


inline EngineGUID::EngineGUID(uint32_t id, uint16_t id1, uint16_t id2,
	uint8_t byte_1, uint8_t byte_2, uint8_t byte_3, uint8_t byte_4, uint8_t byte_5, uint8_t byte_6, uint8_t byte_7, uint8_t byte_8)
{
	hi = (static_cast<uint64_t>(id) << 32) | (static_cast<uint64_t>(id1) << 16 | static_cast<uint64_t>(id2));

	union {
		uint8_t as_bytes[8];
		uint64_t as_int64;
	};

	as_bytes[0] = byte_1;
	as_bytes[1] = byte_2;
	as_bytes[2] = byte_3;
	as_bytes[3] = byte_4;
	as_bytes[4] = byte_5;
	as_bytes[5] = byte_6;
	as_bytes[6] = byte_7;
	as_bytes[7] = byte_8;

	low = as_int64;
}

inline bool EngineGUID::operator==(const EngineGUID& rhs) const
{
	return hi == rhs.hi && low == rhs.low;
}

inline bool EngineGUID::operator!=(const EngineGUID& rhs) const
{
	return hi != rhs.hi || low != rhs.low;
}

inline bool EngineGUID::operator<(const EngineGUID& rhs) const
{
	return hi == rhs.hi ? (low < rhs.low) : (hi < rhs.hi);
}


#endif // !_X_GUID_I_H_
