#pragma once

#ifndef X_TEXTURE_LOADER_PNG_H_
#define X_TEXTURE_LOADER_PNG_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace PNG
{
#define VALIDATE_IDAT_CRC 1

    class XTexLoaderPNG : public ITextureFmt
    {
        static const int32_t BLOCK_SIZE = 1024 * 128;              // save as 128kb chunks.
        static const int32_t IO_READ_BLOCK_SIZE = (1024 * 4) * 4; // read the IDAT in max of 16kb chunks regardless of it's size.

    public:
        static const ImgFileFormat::Enum SRC_FMT = ImgFileFormat::PNG;
        static const char* EXTENSION;

    public:
        IMGLIB_EXPORT XTexLoaderPNG();
        IMGLIB_EXPORT ~XTexLoaderPNG();

        IMGLIB_EXPORT static bool isValidData(const DataVec& fileData);

        // ITextureFmt
        IMGLIB_EXPORT virtual const char* getExtension(void) const X_FINAL;
        IMGLIB_EXPORT virtual ImgFileFormat::Enum getSrcFmt(void) const X_FINAL;
        IMGLIB_EXPORT virtual bool canLoadFile(const core::Path<char>& path) const X_FINAL;
        IMGLIB_EXPORT virtual bool canLoadFile(const DataVec& fileData) const X_FINAL;
        IMGLIB_EXPORT virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;

        IMGLIB_EXPORT virtual bool canWrite(void) const X_FINAL
        {
            return true;
        }
        IMGLIB_EXPORT virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;
        // ~ITextureFmt

    private:
        static bool LoadChucksIDAT(core::MemoryArenaBase* swapArea, core::Crc32* pCrc, core::XFile* file,
            int32_t idatBlockLength, XTextureFile& imgFile);
    };

} // namespace PNG

X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_PNG_H_