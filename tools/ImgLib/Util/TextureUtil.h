#pragma once


#ifndef X_TEXTURE_UTIL_H_
#define X_TEXTURE_UTIL_H_

#include <ITexture.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
)

X_NAMESPACE_BEGIN(texture)

namespace Util
{

	IMGLIB_EXPORT bool hasAlpha(Texturefmt::Enum fmt);
	IMGLIB_EXPORT bool isDxt(Texturefmt::Enum fmt);
	IMGLIB_EXPORT bool isTypeless(Texturefmt::Enum fmt);
	IMGLIB_EXPORT bool isBRG(Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t maxMipsForSize(uint32_t Width, uint32_t Height);
	IMGLIB_EXPORT uint32_t bitsPerPixel(Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t dxtBytesPerBlock(Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t dataSize(uint32_t width, uint32_t height, Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t dataSize(uint32_t width, uint32_t height, uint32_t mips, Texturefmt::Enum fmt);
	IMGLIB_EXPORT uint32_t dataSize(uint32_t width, uint32_t height, uint32_t mips, uint32_t faces, Texturefmt::Enum fmt);
	IMGLIB_EXPORT size_t rowBytes(uint32_t width, uint32_t height, Texturefmt::Enum fmt);
	IMGLIB_EXPORT void faceInfo(uint32_t width, uint32_t height, Texturefmt::Enum fmt,
		size_t* pOutNumBytes, size_t* pOutRowBytes, size_t* pOutNumRows);

	IMGLIB_EXPORT Texturefmt::Enum TexFmtFromStr(const char* pStr);
	IMGLIB_EXPORT ImgFileFormat::Enum resolveSrcfmt(const core::Array<uint8_t>& fileData);
	IMGLIB_EXPORT bool writeSupported(ImgFileFormat::Enum fmt);
	IMGLIB_EXPORT const char* getExtension(ImgFileFormat::Enum fmt);
	IMGLIB_EXPORT bool flipVertical(XTextureFile& img, core::MemoryArenaBase* swap);

	// feel like these bgrToRgb, should not really be here / have better names.
	IMGLIB_EXPORT bool bgrToRgb(XTextureFile& img, core::MemoryArenaBase* swap);
	IMGLIB_EXPORT bool bgrToRgb(const XTextureFile& img, XTextureFile& imgOut, core::MemoryArenaBase* swap);

	IMGLIB_EXPORT bool loadImage(core::MemoryArenaBase* swap, const core::Array<uint8_t>& fileData, ImgFileFormat::Enum fmt, XTextureFile& img);
	IMGLIB_EXPORT bool loadImage(core::MemoryArenaBase* swap, core::XFile* pFile, ImgFileFormat::Enum fmt, XTextureFile& img);

	bool saveImage(core::MemoryArenaBase* swap, core::XFile* pFile, ImgFileFormat::Enum fmt, const XTextureFile& img);

} // namespace Util

X_NAMESPACE_END

#endif // !X_TEXTURE_UTIL_H_