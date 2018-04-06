#include "stdafx.h"
#include "TextureLoaderCI.h"

#include <IFileSys.h>
#include <ITexture.h>
#include <ICi.h>

#include "TextureFile.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{
    namespace
    {
        // add a static assert here that TEX_MAX_DIMENSIONS is not bigger than what this format
        // can store.
        X_ENSURE_LE(TEX_MAX_DIMENSIONS, CI_MAX_DIMENSIONS, "TEX_MAX_DIMENSIONS exceeds what CI image can store");
        X_ENSURE_LE(TEX_MAX_MIPS, CI_MAX_MIPS, "TEX_MAX_MIPS exceeds what CI image can store");
        X_ENSURE_LE(TEX_MAX_FACES, CI_MAX_FACES, "TEX_MAX_FACES exceeds what CI image can store");
    } // namespace

    const char* XTexLoaderCI::EXTENSION = CI_FILE_EXTENSION;

    XTexLoaderCI::XTexLoaderCI()
    {
    }

    XTexLoaderCI::~XTexLoaderCI()
    {
    }

    bool XTexLoaderCI::isValidData(const DataVec& fileData)
    {
        if (fileData.size() < sizeof(CITexureHeader)) {
            return false;
        }

        const CITexureHeader* pHdr = reinterpret_cast<const CITexureHeader*>(fileData.data());

        return pHdr->fourCC == CI_FOURCC;
    }

    // ITextureFmt
    const char* XTexLoaderCI::getExtension(void) const
    {
        return CI_FILE_EXTENSION;
    }

    ImgFileFormat::Enum XTexLoaderCI::getSrcFmt(void) const
    {
        return ImgFileFormat::CI;
    }

    bool XTexLoaderCI::canLoadFile(const core::Path<char>& path) const
    {
        return core::strUtil::IsEqual(CI_FILE_EXTENSION, path.extension());
    }

    bool XTexLoaderCI::canLoadFile(const DataVec& fileData) const
    {
        return isValidData(fileData);
    }

    bool XTexLoaderCI::loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_UNUSED(swapArena);

        return loadTexture(file, imgFile);
    }

    bool XTexLoaderCI::loadTexture(core::XFile* file, XTextureFile& imgFile)
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pLog);
        X_ASSERT_NOT_NULL(file);

        CITexureHeader hdr;

        // file system will report read error in log.
        // but lets return here also.
        if (file->readObj(hdr) != sizeof(hdr)) {
            return false;
        }

        if (!hdr.isValid()) {
            X_ERROR("TextureCI", "header is not valid");
            return false;
        }

        if (hdr.version != CI_VERSION) {
            X_ERROR("TextureCI", "version is invalid. provided: %" PRIu8 " expected: %" PRIu8, hdr.version, CI_VERSION);
            return false;
        }

        // ok that's all the checks we need to do
        // since image has passed all the required checks to become a CI.
        // but if someone decides to make a tool that can make CI.
        // they might try load a larger image, so lets just check dimensions.
        if (hdr.height > TEX_MAX_DIMENSIONS || hdr.width > TEX_MAX_DIMENSIONS) {
            X_ERROR("TextureCI", "invalid image dimensions. provided: %" PRIu16 "x%" PRIu16 " max: %" PRIu32 "x%" PRIu32,
                hdr.height, hdr.width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);
            return false;
        }

        if (hdr.height < TEX_MIN_DIMENSIONS || hdr.width < TEX_MIN_DIMENSIONS) {
            X_ERROR("TextureCI", "invalid image dimensions. provided: %" PRIu16 "x%" PRIu16 " min: %" PRIu32 "x%" PRIu32,
                hdr.height, hdr.width, TEX_MIN_DIMENSIONS, TEX_MIN_DIMENSIONS);
            return false;
        }

        if (!core::bitUtil::IsPowerOfTwo(hdr.height) || !core::bitUtil::IsPowerOfTwo(hdr.width)) {
            X_ERROR("TextureCI", "invalid image dimensions, must be power of two. provided: %" PRIu16 "x%" PRIu16,
                hdr.height, hdr.width);
            return false;
        }

        if ((hdr.DataSize % hdr.FaceSize) != 0) {
            X_ERROR("TextureCI", "data size is not a multiple of facesize");
            return false;
        }

        // flags
        TextureFlags flags = hdr.Flags;
        // tools not required to set this
        // so just set it here so it's reliable.
        // since this is the only place it matters.
        flags.Set(TexFlag::CI_IMG);

        // set the info
        imgFile.setWidth(hdr.width);
        imgFile.setHeigth(hdr.height);
        imgFile.setNumMips(hdr.Mips);
        imgFile.setNumFaces(hdr.Faces); // 1 for 2D 6 for a cube.
        imgFile.setDepth(1);            /// We Don't allow volume texture loading yet.
        imgFile.setFlags(flags);
        imgFile.setFormat(hdr.format);
        imgFile.resize();

        size_t bytesRead = file->read(imgFile.getFace(0), hdr.DataSize);

        if (bytesRead != hdr.DataSize) {
            X_ERROR("TextureCI", "failed to read image data from CIImage. got: %x wanted: %x bytes",
                bytesRead, hdr.DataSize);
            return false;
        }

        if (file->remainingBytes() != 0) {
            X_ERROR("TextureCI", "read fail, bytes left in file: %" PRIu64, file->remainingBytes());
            return false;
        }

        return true;
    }
    // ~ITextureFmt

} // namespace CI

X_NAMESPACE_END