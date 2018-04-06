#pragma once

#ifndef X_TEXTURE_LOADER_CI_H_
#define X_TEXTURE_LOADER_CI_H_

#include "ITexture.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{
    class IMGLIB_EXPORT XTexLoaderCI : public ITextureFmt
    {
    public:
        static const ImgFileFormat::Enum SRC_FMT = ImgFileFormat::CI;
        static const char* EXTENSION;

    public:
        XTexLoaderCI();
        ~XTexLoaderCI();

        static bool isValidData(const DataVec& fileData);

        // ITextureFmt
        virtual const char* getExtension(void) const X_FINAL;
        virtual ImgFileFormat::Enum getSrcFmt(void) const X_FINAL;
        virtual bool canLoadFile(const core::Path<char>& path) const X_FINAL;
        virtual bool canLoadFile(const DataVec& fileData) const X_FINAL;
        virtual bool loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;
        bool loadTexture(core::XFile* file, XTextureFile& imgFile);

        virtual bool canWrite(void) const X_FINAL
        {
            return true;
        }
        virtual bool saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena) X_FINAL;

        // ~ITextureFmt

    private:
    };

} // namespace CI

X_NAMESPACE_END

#endif // X_TEXTURE_LOADER_CI_H_