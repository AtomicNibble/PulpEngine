#pragma once


X_NAMESPACE_BEGIN(core)

inline uint16_t operator "" _u16(unsigned long long value)
{
	return static_cast<uint16_t>(value);
}


X_NAMESPACE_END