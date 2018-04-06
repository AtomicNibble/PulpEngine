#pragma once

#ifndef X_TEXTURE_LOADER_DDS_H_
#define X_TEXTURE_LOADER_DDS_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace DDS
{
    class IMGLIB_EXPORT XTexLoaderDDS : public ITextureFmt
    {
    public:
        static const ImgFileFormat::Enum SRC_FMT = ImgFileFormat::DDS;
        static const char* EXTENSION;

    public:
        XTexLoaderDDS();
        ~XTexLoaderDDS();

        static bool isValidData(const DataVec& fileData);

        // ITextureFmt
        virtual const char* getExtension(void) const X_FINAL;
        virtual ImgFileFormat::Enum getSrcFmt(void) const X_FINAL;
        virtual bool canLoadFile(const core::Path<char>& path) const X_FINAL;
        virtual bool canLoadFile(const DataVec& fileData) const X_FINAL;
        virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;

        virtual bool canWrite(void) const X_FINAL
        {
            return true;
        }
        virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;

        // ~ITextureFmt

    private:
    };

} // namespace DDS

X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_DDS_H_