#pragma once


#ifndef X_TEXTURE_LOADER_TGA_H_
#define X_TEXTURE_LOADER_TGA_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace TGA
{

	class XTexLoaderTGA : public ITextureFmt
	{
	public:
		static const ImgFileFormat::Enum SRC_FMT = ImgFileFormat::TGA;
		static const char* EXTENSION;

	public:
		IMGLIB_EXPORT XTexLoaderTGA();
		IMGLIB_EXPORT ~XTexLoaderTGA();

		IMGLIB_EXPORT static bool isValidData(const DataVec& fileData);

		// ITextureFmt
		IMGLIB_EXPORT virtual const char* getExtension(void) const X_FINAL;
		IMGLIB_EXPORT virtual ImgFileFormat::Enum getSrcFmt(void) const X_FINAL;
		IMGLIB_EXPORT virtual bool canLoadFile(const core::Path<char>& path) const X_FINAL;
		IMGLIB_EXPORT virtual bool canLoadFile(const DataVec& fileData) const X_FINAL;
		IMGLIB_EXPORT virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;

		// ~ITextureFmt

	private:
		static bool isValidImageType(uint32_t type);
		static bool isColorMap(uint32_t type);
		static bool isBGR(uint32_t type);
		static bool isMono(uint32_t type);
		static bool isRle(uint32_t type);

		static bool isRightToLeft(uint32_t descriptor);
		static bool isTopToBottom(uint32_t descriptor);


	};

} // namespace TGA


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_TGA_H_