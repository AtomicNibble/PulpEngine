#pragma once


#ifndef X_TEXTURE_LOADER_TGA_H_
#define X_TEXTURE_LOADER_TGA_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace TGA
{

	class IMGLIB_EXPORT XTexLoaderTGA : public ITextureFmt
	{
	public:
		static const ImgFileFormat::Enum SRC_FMT = ImgFileFormat::TGA;

	public:
		XTexLoaderTGA();
		~XTexLoaderTGA();

		static bool isValidData(const DataVec& fileData);

		// ITextureFmt
		virtual const char* getExtension(void) const X_OVERRIDE;
		virtual ImgFileFormat::Enum getSrcFmt(void) const X_OVERRIDE;
		virtual bool canLoadFile(const core::Path<char>& path) const X_OVERRIDE;
		virtual bool canLoadFile(const DataVec& fileData) const X_OVERRIDE;
		virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_OVERRIDE;

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