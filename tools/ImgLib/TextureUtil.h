#pragma once


#ifndef X_TEXTURE_UTIL_H_
#define X_TEXTURE_UTIL_H_

#include <ITexture.h>

X_NAMESPACE_BEGIN(texture)

namespace Util
{

	IMGLIB_EXPORT bool hasAlpha(Texturefmt::Enum fmt);
	IMGLIB_EXPORT bool isDxt(Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t maxMipsForSize(uint32_t Width, uint32_t Height);
	IMGLIB_EXPORT uint32_t bitsPerPixel(Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t dxtBytesPerBlock(Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t dataSize(uint32_t width, uint32_t height, Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t dataSize(uint32_t width, uint32_t height, uint32_t mips, Texturefmt::Enum fmt);
	IMGLIB_EXPORT size_t rowBytes(uint32_t width, uint32_t height, Texturefmt::Enum fmt);
	IMGLIB_EXPORT void faceInfo(uint32_t width, uint32_t height, Texturefmt::Enum fmt,
		size_t* pOutNumBytes, size_t* pOutRowBytes, size_t* pOutNumRows);

	IMGLIB_EXPORT Texturefmt::Enum TexFmtFromStr(const char* pStr);


} // namespace Util

X_NAMESPACE_END

#endif // !X_TEXTURE_UTIL_H_