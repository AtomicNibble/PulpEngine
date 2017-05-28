#pragma once


#ifndef X_TEXTURE_LOADER_PNG_H_
#define X_TEXTURE_LOADER_PNG_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace PNG
{
	#define VALIDATE_IDAT_CRC 1

	class IMGLIB_EXPORT XTexLoaderPNG : public ITextureFmt
	{
		static const int32_t BLOCK_SIZE = 1024 * 32; // save as 32kb chunks.
		static const int32_t IO_READ_BLOCK_SIZE = (1024 * 4) * 4; // read the IDAT in max of 16kb chunks regardless of it's size.

	public:
		static const ImgFileFormat::Enum SRC_FMT = ImgFileFormat::PNG;

	public:
		XTexLoaderPNG();
		~XTexLoaderPNG();

		static bool isValidData(const DataVec& fileData);

		// ITextureFmt
		virtual const char* getExtension(void) const X_FINAL;
		virtual ImgFileFormat::Enum getSrcFmt(void) const X_FINAL;
		virtual bool canLoadFile(const core::Path<char>& path) const X_FINAL;
		virtual bool canLoadFile(const DataVec& fileData) const X_FINAL;
		virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;

		virtual bool canWrite(void) const X_FINAL { return true; }
		virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;
		// ~ITextureFmt

	private:
		static bool LoadChucksIDAT(core::MemoryArenaBase* swapArea, core::Crc32* pCrc, core::XFile* file,
			int32_t idatBlockLength, XTextureFile& imgFile);
		
	};

} // namespace PNG


X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_PNG_H_