#pragma once


#ifndef X_TEXTURE_UTIL_H_
#define X_TEXTURE_UTIL_H_

#include <ITexture.h>

X_NAMESPACE_BEGIN(texture)

namespace Util
{

	bool isDxt(Texturefmt::Enum fmt);
	uint32_t numMipsMips(uint32_t Width, uint32_t Height);
	uint32_t bitsPerPixel(Texturefmt::Enum fmt);
	uint32_t dxtBytesPerBlock(Texturefmt::Enum fmt);
	uint32_t dataSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, Texturefmt::Enum fmt);
	void faceInfo(uint32_t width, uint32_t height, Texturefmt::Enum fmt,
		size_t* pOutNumBytes, size_t* pOutRowBytes, size_t* pOutNumRows);

	Texturefmt::Enum TexFmtFromStr(const char* pStr);


} // namespace Util

X_NAMESPACE_END

#endif // !X_TEXTURE_UTIL_H_