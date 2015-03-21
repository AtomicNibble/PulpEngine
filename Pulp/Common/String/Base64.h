#pragma once


#ifndef X_BASE64_H_
#define X_BASE64_H_

#include "StrRef.h"

X_NAMESPACE_BEGIN(core)

class Base64
{
	static uint8 EncodingAlphabet[64];
	static uint8 DecodingAlphabet[256];

	X_NO_CREATE(Base64);
public:

	static core::string Encode(const core::string& str);
	static core::string Decode(const core::string& str);

};


X_NAMESPACE_END

#endif // X_BASE64_H_