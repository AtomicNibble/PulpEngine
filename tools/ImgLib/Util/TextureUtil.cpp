#include "stdafx.h"
#include "TextureUtil.h"
#include "TextureFile.h"

#include <IFileSys.h>

#include "Fmts\TextureLoaderCI.h"
#include "Fmts\TextureLoaderDDS.h"
#include "Fmts\TextureLoaderJPG.h"
#include "Fmts\TextureLoaderPNG.h"
#include "Fmts\TextureLoaderPSD.h"
#include "Fmts\TextureLoaderTGA.h"

#include <Hashing\Fnva1Hash.h>

#include <Memory/AllocationPolicies/LinearAllocator.h>

X_NAMESPACE_BEGIN(texture)

namespace Util
{
    namespace
    {
        static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");

        const size_t MAX_FMT_CLASS_SIZE = core::Max<size_t>(
                                              core::Max(
                                                  core::Max(
                                                      core::Max(
                                                          core::Max(
                                                              core::Max(
                                                                  sizeof(DDS::XTexLoaderDDS),
                                                                  sizeof(CI::XTexLoaderCI)),
                                                              sizeof(JPG::XTexLoaderJPG)),
                                                          sizeof(TGA::XTexLoaderTGA)),
                                                      sizeof(PSD::XTexLoaderPSD)),
                                                  sizeof(PNG::XTexLoaderPNG)),
                                              8)
                                          + 64;

        ITextureFmt* Allocfmt(core::LinearAllocator* pAllocator, ImgFileFormat::Enum inputFileFmt)
        {
            ITextureFmt* pFmt = nullptr;

            static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img fmts? this code needs updating.");

            switch (inputFileFmt) {
                case ImgFileFormat::DDS:
                    pFmt = core::Mem::Construct<DDS::XTexLoaderDDS>(pAllocator->allocate(sizeof(DDS::XTexLoaderDDS), X_ALIGN_OF(DDS::XTexLoaderDDS), 0));
                    break;
                case ImgFileFormat::CI:
                    pFmt = core::Mem::Construct<CI::XTexLoaderCI>(pAllocator->allocate(sizeof(CI::XTexLoaderCI), X_ALIGN_OF(CI::XTexLoaderCI), 0));
                    break;
                case ImgFileFormat::JPG:
                    pFmt = core::Mem::Construct<JPG::XTexLoaderJPG>(pAllocator->allocate(sizeof(JPG::XTexLoaderJPG), X_ALIGN_OF(JPG::XTexLoaderJPG), 0));
                    break;
                case ImgFileFormat::TGA:
                    pFmt = core::Mem::Construct<TGA::XTexLoaderTGA>(pAllocator->allocate(sizeof(TGA::XTexLoaderTGA), X_ALIGN_OF(TGA::XTexLoaderTGA), 0));
                    break;
                case ImgFileFormat::PSD:
                    pFmt = core::Mem::Construct<PSD::XTexLoaderPSD>(pAllocator->allocate(sizeof(PSD::XTexLoaderPSD), X_ALIGN_OF(PSD::XTexLoaderPSD), 0));
                    break;
                case ImgFileFormat::PNG:
                    pFmt = core::Mem::Construct<PNG::XTexLoaderPNG>(pAllocator->allocate(sizeof(PNG::XTexLoaderPNG), X_ALIGN_OF(PNG::XTexLoaderPNG), 0));
                    break;

                default:
                    X_ASSERT_UNREACHABLE();
                    break;
            }

            return pFmt;
        }

        void FreeFmt(ITextureFmt* pFmt)
        {
            core::Mem::Destruct<ITextureFmt>(pFmt);
        }

    } // namespace

    uint32_t maxMipsForSize(uint32_t Width, uint32_t Height)
    {
        uint32_t Biggest = core::Max<uint32_t>(Width, Height);
        uint32_t mips = 0;
        while (Biggest > 0) {
            mips++;
            Biggest >>= 1;
        }
        return mips;
    }

    bool hasAlpha(Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::BC3:
            case Texturefmt::BC3_SRGB:
            case Texturefmt::BC3_TYPELESS:
            case Texturefmt::BC7:
            case Texturefmt::BC7_TYPELESS:
            case Texturefmt::BC7_SRGB:
            case Texturefmt::R8G8B8A8:
            case Texturefmt::R8G8B8A8_SRGB:
            case Texturefmt::R8G8B8A8_SNORM:
            case Texturefmt::R8G8B8A8_TYPELESS:
            case Texturefmt::R8G8B8A8_SINT:
            case Texturefmt::R8G8B8A8_UINT:
            case Texturefmt::A8R8G8B8:
            case Texturefmt::B8G8R8A8:
            case Texturefmt::B8G8R8A8_SRGB:
            case Texturefmt::B8G8R8A8_TYPELESS:
            case Texturefmt::R10G10B10A2:
            case Texturefmt::R16G16B16A16_FLOAT:
                return true;

            default:
                break;
        }

        return false;
    }

    bool isDxt(Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::BC1:
            case Texturefmt::BC1_SRGB:
            case Texturefmt::BC1_TYPELESS:
            case Texturefmt::BC4:
            case Texturefmt::BC4_SNORM:
            case Texturefmt::BC4_TYPELESS:
            case Texturefmt::BC2:
            case Texturefmt::BC2_SRGB:
            case Texturefmt::BC2_TYPELESS:
            case Texturefmt::BC3:
            case Texturefmt::BC3_SRGB:
            case Texturefmt::BC3_TYPELESS:
            case Texturefmt::BC5:
            case Texturefmt::BC5_SNORM:
            case Texturefmt::BC5_TYPELESS:
            case Texturefmt::BC6:
            case Texturefmt::BC6_TYPELESS:
            case Texturefmt::BC6_SF16:
            case Texturefmt::BC7:
            case Texturefmt::BC7_TYPELESS:
            case Texturefmt::BC7_SRGB:
            case Texturefmt::ATI2:
            case Texturefmt::ATI2_XY:
                return true;
            default:
                break;
        }
        return false;
    }

    bool isTypeless(Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::R8G8_TYPELESS:
            case Texturefmt::R16G16_TYPELESS:
            case Texturefmt::R8G8B8A8_TYPELESS:
            case Texturefmt::B8G8R8A8_TYPELESS:
            case Texturefmt::BC1_TYPELESS:
            case Texturefmt::BC2_TYPELESS:
            case Texturefmt::BC3_TYPELESS:
            case Texturefmt::BC4_TYPELESS:
            case Texturefmt::BC5_TYPELESS:
            case Texturefmt::BC6_TYPELESS:
            case Texturefmt::BC7_TYPELESS:
            case Texturefmt::R10G10B10A2_TYPELESS:
            case Texturefmt::R24G8_TYPELESS:
                return true;
            default:
                break;
        }
        return false;
    }

    bool isBRG(Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::B8G8R8:
            case Texturefmt::B8G8R8A8:
            case Texturefmt::B8G8R8A8_SRGB:
            case Texturefmt::B8G8R8A8_TYPELESS:
                return true;
            default:
                return false;
        }
    }

    uint32_t bitsPerPixel(Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::BC1:
            case Texturefmt::BC1_SRGB:
            case Texturefmt::BC1_TYPELESS:
            case Texturefmt::BC4:
            case Texturefmt::BC4_SNORM:
            case Texturefmt::BC4_TYPELESS:
                return 4;

            case Texturefmt::A8:
                return 8;

            case Texturefmt::BC2:
            case Texturefmt::BC2_SRGB:
            case Texturefmt::BC2_TYPELESS:
            case Texturefmt::BC3:
            case Texturefmt::BC3_SRGB:
            case Texturefmt::BC3_TYPELESS:
            case Texturefmt::BC5:
            case Texturefmt::BC5_SNORM:
            case Texturefmt::BC5_TYPELESS:
            case Texturefmt::BC6:
            case Texturefmt::BC6_TYPELESS:
            case Texturefmt::BC6_SF16:
            case Texturefmt::BC7:
            case Texturefmt::BC7_TYPELESS:
            case Texturefmt::BC7_SRGB:
                return 8;

            case Texturefmt::ATI2:
            case Texturefmt::ATI2_XY:
                return 8;

            case Texturefmt::R8G8:
            case Texturefmt::R8G8_TYPELESS:
            case Texturefmt::R8G8_SNORM:
            case Texturefmt::R8G8_UNIT:
            case Texturefmt::R8G8_SINT:
                return 16;

            case Texturefmt::R8G8B8:
            case Texturefmt::B8G8R8:
                return 24;

            case Texturefmt::R16G16_FLOAT:
            case Texturefmt::R16G16:
            case Texturefmt::R16G16_SRGB:
            case Texturefmt::R16G16_SNORM:
            case Texturefmt::R16G16_SINT:
            case Texturefmt::R16G16_UINT:
            case Texturefmt::R16G16_TYPELESS:

            case Texturefmt::R8G8B8A8:
            case Texturefmt::R8G8B8A8_SRGB:
            case Texturefmt::R8G8B8A8_SNORM:
            case Texturefmt::R8G8B8A8_TYPELESS:
            case Texturefmt::R8G8B8A8_SINT:
            case Texturefmt::R8G8B8A8_UINT:

            case Texturefmt::A8R8G8B8:

            case Texturefmt::B8G8R8A8:
            case Texturefmt::B8G8R8A8_SRGB:
            case Texturefmt::B8G8R8A8_TYPELESS:

            case Texturefmt::R24G8_TYPELESS:
            case Texturefmt::D24_UNORM_S8_UNIT:
            case Texturefmt::D32_FLOAT:
                return 32;

            case Texturefmt::R10G10B10A2:
                return 32;

            case Texturefmt::R16G16B16A16_FLOAT:
                return 64;

            default:
                break;
        }
        X_ASSERT_UNREACHABLE();
        return 0;
    }

    uint32_t dxtBytesPerBlock(Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::BC1:
            case Texturefmt::BC1_SRGB:
            case Texturefmt::BC1_TYPELESS:
            case Texturefmt::BC4:
            case Texturefmt::BC4_SNORM:
            case Texturefmt::BC4_TYPELESS:
                return 8;

            case Texturefmt::BC2:
            case Texturefmt::BC2_SRGB:
            case Texturefmt::BC2_TYPELESS:
            case Texturefmt::BC3:
            case Texturefmt::BC3_SRGB:
            case Texturefmt::BC3_TYPELESS:
            case Texturefmt::BC5:
            case Texturefmt::BC5_SNORM:
            case Texturefmt::BC5_TYPELESS:
            case Texturefmt::BC6:
            case Texturefmt::BC6_TYPELESS:
            case Texturefmt::BC6_SF16:
            case Texturefmt::BC7:
            case Texturefmt::BC7_TYPELESS:
            case Texturefmt::BC7_SRGB:
                return 16;

            case Texturefmt::ATI2:
            case Texturefmt::ATI2_XY:
                return 16;

            default:
                break;
        }

        X_ASSERT_UNREACHABLE();
        return 0;
    }

    uint32_t dataSize(uint32_t width, uint32_t height, Texturefmt::Enum fmt)
    {
        const uint32_t bits_per_pixel = bitsPerPixel(fmt);
        const bool isDXT = isDxt(fmt);

        uint32_t bytes_per_block = 0;
        uint32_t size = 0;

        if (isDXT) {
            bytes_per_block = dxtBytesPerBlock(fmt);
        }

        width = core::Max(1u, width);
        height = core::Max(1u, height);

        // work out total pixels.
        if (isDXT) {
            // scale to 4x4 pixel blocks.
            size = core::Max(bytes_per_block, ((width + 3) / 4) * ((height + 3) / 4) * bytes_per_block);
        }
        else {
            size = ((bits_per_pixel * width) * height) / 8;
        }

        return size;
    }

    uint32_t dataSize(uint32_t width, uint32_t height,
        uint32_t mips, Texturefmt::Enum fmt)
    {
        X_ASSERT(width > 0, "zero width")
        (width);
        X_ASSERT(height > 0, "zero height")
        (height);

        uint32_t size = 0;
        uint32_t i;

        const uint32_t bits_per_pixel = bitsPerPixel(fmt);
        const bool isDXT = isDxt(fmt);

        uint32_t bytes_per_block = 0;

        if (isDXT) {
            bytes_per_block = dxtBytesPerBlock(fmt);
        }

        for (i = 0; i < mips; i++) {
            width = core::Max(1u, width);
            height = core::Max(1u, height);

            // work out total pixels.
            if (isDXT) {
                // scale to 4x4 pixel blocks.
                size += core::Max(bytes_per_block, ((width + 3) / 4) * ((height + 3) / 4) * bytes_per_block);
            }
            else {
                size += ((bits_per_pixel * width) * height) / 8;
            }

            // shift
            width >>= 1;
            height >>= 1;
        }

        return size;
    }

    uint32_t dataSize(uint32_t width, uint32_t height,
        uint32_t mips, uint32_t faces, Texturefmt::Enum fmt)
    {
        return dataSize(width, height, mips, fmt) * faces;
    }

    size_t rowBytes(uint32_t width, uint32_t height, Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        size_t rowBytes = 0;

        bool blockComp = false;
        bool packed = false;
        bool planar = false;
        size_t bytesPerElem = 0;

        switch (fmt) {
            case Texturefmt::BC1:
            case Texturefmt::BC1_SRGB:
            case Texturefmt::BC1_TYPELESS:
            case Texturefmt::BC4:
            case Texturefmt::BC4_SNORM:
            case Texturefmt::BC4_TYPELESS:
                blockComp = true;
                bytesPerElem = 8;
                break;

            case Texturefmt::BC2:
            case Texturefmt::BC2_SRGB:
            case Texturefmt::BC2_TYPELESS:
            case Texturefmt::BC3:
            case Texturefmt::BC3_SRGB:
            case Texturefmt::BC3_TYPELESS:
            case Texturefmt::BC5:
            case Texturefmt::BC5_SNORM:
            case Texturefmt::BC5_TYPELESS:
            case Texturefmt::BC6:
            case Texturefmt::BC6_TYPELESS:
            case Texturefmt::BC6_SF16:
            case Texturefmt::BC7:
            case Texturefmt::BC7_TYPELESS:
            case Texturefmt::BC7_SRGB:

            case Texturefmt::ATI2:
            case Texturefmt::ATI2_XY:
                blockComp = true;
                bytesPerElem = 16;
                break;

            default:
                break;
        }

        if (blockComp) {
            size_t numBlocksWide = 0;
            if (width > 0) {
                numBlocksWide = core::Max<size_t>(1, (width + 3) / 4);
            }
            size_t numBlocksHigh = 0;
            if (height > 0) {
                numBlocksHigh = core::Max<size_t>(1, (height + 3) / 4);
            }
            rowBytes = numBlocksWide * bytesPerElem;
        }
        else if (packed) {
            rowBytes = ((width + 1) >> 1) * bytesPerElem;
        }
        else if (planar) {
            rowBytes = ((width + 1) >> 1) * bytesPerElem;
        }
        else {
            size_t bpp = bitsPerPixel(fmt);
            rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
        }

        return rowBytes;
    }

    void faceInfo(uint32_t width, uint32_t height, Texturefmt::Enum fmt,
        size_t* pOutNumBytes, size_t* pOutRowBytes, size_t* pOutNumRows)
    {
        X_ASSERT_NOT_NULL(pOutNumBytes);
        X_ASSERT_NOT_NULL(pOutRowBytes);
        X_ASSERT_NOT_NULL(pOutNumRows);

        size_t numBytes = 0;
        size_t rowBytes = 0;
        size_t numRows = 0;

        bool blockComp = false;
        bool packed = false;
        bool planar = false;
        size_t bytesPerElem = 0;

        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::BC1:
            case Texturefmt::BC1_SRGB:
            case Texturefmt::BC1_TYPELESS:
            case Texturefmt::BC4:
            case Texturefmt::BC4_SNORM:
            case Texturefmt::BC4_TYPELESS:
                blockComp = true;
                bytesPerElem = 8;
                break;

            case Texturefmt::BC2:
            case Texturefmt::BC2_SRGB:
            case Texturefmt::BC2_TYPELESS:
            case Texturefmt::BC3:
            case Texturefmt::BC3_SRGB:
            case Texturefmt::BC3_TYPELESS:
            case Texturefmt::BC5:
            case Texturefmt::BC5_SNORM:
            case Texturefmt::BC5_TYPELESS:
            case Texturefmt::BC6:
            case Texturefmt::BC6_TYPELESS:
            case Texturefmt::BC6_SF16:
            case Texturefmt::BC7:
            case Texturefmt::BC7_TYPELESS:
            case Texturefmt::BC7_SRGB:

            case Texturefmt::ATI2:
            case Texturefmt::ATI2_XY:
                blockComp = true;
                bytesPerElem = 16;
                break;

            default:
                break;
        }

        if (blockComp) {
            size_t numBlocksWide = 0;
            if (width > 0) {
                numBlocksWide = core::Max<size_t>(1, (width + 3) / 4);
            }
            size_t numBlocksHigh = 0;
            if (height > 0) {
                numBlocksHigh = core::Max<size_t>(1, (height + 3) / 4);
            }
            rowBytes = numBlocksWide * bytesPerElem;
            numRows = numBlocksHigh;
            numBytes = rowBytes * numBlocksHigh;
        }
        else if (packed) {
            rowBytes = ((width + 1) >> 1) * bytesPerElem;
            numRows = height;
            numBytes = rowBytes * height;
        }
        else if (planar) {
            rowBytes = ((width + 1) >> 1) * bytesPerElem;
            numBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
            numRows = height + ((height + 1) >> 1);
        }
        else {
            size_t bpp = bitsPerPixel(fmt);
            rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
            numRows = height;
            numBytes = rowBytes * height;
        }

        *pOutNumBytes = numBytes;
        *pOutRowBytes = rowBytes;
        *pOutNumRows = numRows;
    }

    Texturefmt::Enum TexFmtFromStr(const char* pStr)
    {
        X_ASSERT_NOT_NULL(pStr);

        // force input to upper case.
        core::StackString<64, char> strUpper(pStr);
        strUpper.toUpper();

        // everything is 2 or mroe currently.
        if (strUpper.length() < 2) {
            return Texturefmt::UNKNOWN;
        }

        using namespace core::Hash::Literals;

        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length())) {
            case "A8"_fnv1a:
                return Texturefmt::A8;

            case "R8G8"_fnv1a:
                return Texturefmt::R8G8;
            case "R8G8_TYPELESS"_fnv1a:
                return Texturefmt::R8G8_TYPELESS;
            case "R8G8_SNORM"_fnv1a:
                return Texturefmt::R8G8_SNORM;
            case "R8G8_UNIT"_fnv1a:
                return Texturefmt::R8G8_UNIT;
            case "R8G8_SINT"_fnv1a:
                return Texturefmt::R8G8_SINT;

            case "R16G16_FLOAT"_fnv1a:
                return Texturefmt::R16G16_FLOAT;
            case "R16G16"_fnv1a:
                return Texturefmt::R16G16;
            case "R16G16_SRGB"_fnv1a:
                return Texturefmt::R16G16_SRGB;
            case "R16G16_SNORM"_fnv1a:
                return Texturefmt::R16G16_SNORM;
            case "R16G16_SINT"_fnv1a:
                return Texturefmt::R16G16_SINT;
            case "R16G16_UINT"_fnv1a:
                return Texturefmt::R16G16_UINT;
            case "R16G16_TYPELESS"_fnv1a:
                return Texturefmt::R16G16_TYPELESS;

            case "R8G8B8"_fnv1a:
                return Texturefmt::R8G8B8;
            case "B8G8R8"_fnv1a:
                return Texturefmt::B8G8R8;

            case "R8G8B8A8"_fnv1a:
                return Texturefmt::R8G8B8A8;
            case "R8G8B8A8_SRGB"_fnv1a:
                return Texturefmt::R8G8B8A8_SRGB;
            case "R8G8B8A8_SNORM"_fnv1a:
                return Texturefmt::R8G8B8A8_SNORM;
            case "R8G8B8A8_TYPELESS"_fnv1a:
                return Texturefmt::R8G8B8A8_TYPELESS;
            case "R8G8B8A8_SINT"_fnv1a:
                return Texturefmt::R8G8B8A8_SINT;
            case "R8G8B8A8_UINT"_fnv1a:
                return Texturefmt::R8G8B8A8_UINT;

            case "B8G8R8A8"_fnv1a:
                return Texturefmt::B8G8R8A8;
            case "B8G8R8A8_SRGB"_fnv1a:
                return Texturefmt::B8G8R8A8_SRGB;
            case "B8G8R8A8_TYPELESS"_fnv1a:
                return Texturefmt::B8G8R8A8_TYPELESS;

            case "ATI2"_fnv1a:
                return Texturefmt::ATI2;
            case "ATI2_XY"_fnv1a:
                return Texturefmt::ATI2_XY;

            case "BC1"_fnv1a:
                return Texturefmt::BC1;
            case "BC1_SRGB"_fnv1a:
                return Texturefmt::BC1_SRGB;
            case "BC1_TYPELESS"_fnv1a:
                return Texturefmt::BC1_TYPELESS;

            case "BC2"_fnv1a:
                return Texturefmt::BC2;
            case "BC2_SRGB"_fnv1a:
                return Texturefmt::BC2_SRGB;
            case "BC2_TYPELESS"_fnv1a:
                return Texturefmt::BC2_TYPELESS;

            case "BC3"_fnv1a:
                return Texturefmt::BC3;
            case "BC3_SRGB"_fnv1a:
                return Texturefmt::BC3_SRGB;
            case "BC3_TYPELESS"_fnv1a:
                return Texturefmt::BC3_TYPELESS;

            case "BC4"_fnv1a:
                return Texturefmt::BC4;
            case "BC4_SNORM"_fnv1a:
                return Texturefmt::BC4_SNORM;
            case "BC4_TYPELESS"_fnv1a:
                return Texturefmt::BC4_TYPELESS;

            case "BC5"_fnv1a:
                return Texturefmt::BC5;
            case "BC5_SNORM"_fnv1a:
                return Texturefmt::BC5_SNORM;
            case "BC5_TYPELESS"_fnv1a:
                return Texturefmt::BC5_TYPELESS;

            case "BC6"_fnv1a:
                return Texturefmt::BC6;
            case "BC6_SF16"_fnv1a:
                return Texturefmt::BC6_SF16;
            case "BC6_TYPELESS"_fnv1a:
                return Texturefmt::BC6_TYPELESS;

            case "BC7"_fnv1a:
                return Texturefmt::BC7;
            case "BC7_SRGB"_fnv1a:
                return Texturefmt::BC7_SRGB;
            case "BC7_TYPELESS"_fnv1a:
                return Texturefmt::BC7_TYPELESS;

            case "R10G10B10A2"_fnv1a:
                return Texturefmt::R10G10B10A2;
            case "R10G10B10A2_UINT"_fnv1a:
                return Texturefmt::R10G10B10A2_UINT;
            case "R10G10B10A2_TYPELESS"_fnv1a:
                return Texturefmt::R10G10B10A2_TYPELESS;

            case "R16G16B16A16_FLOAT"_fnv1a:
                return Texturefmt::R16G16B16A16_FLOAT;

            case "R24G8_TYPELESS"_fnv1a:
                return Texturefmt::R24G8_TYPELESS;
            case "D24_UNORM_S8_UNIT"_fnv1a:
                return Texturefmt::D24_UNORM_S8_UNIT;
            case "D32_FLOAT"_fnv1a:
                return Texturefmt::D32_FLOAT;

            default:
                // this is here incase i've forgot to add a new format
                // into this function.
                X_ASSERT_UNREACHABLE();
                return Texturefmt::UNKNOWN;
        }
    }

    ImgFileFormat::Enum resolveSrcfmt(const core::Array<uint8_t>& fileData)
    {
        static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");

        if (CI::XTexLoaderCI::isValidData(fileData)) {
            return CI::XTexLoaderCI::SRC_FMT;
        }
        if (DDS::XTexLoaderDDS::isValidData(fileData)) {
            return DDS::XTexLoaderDDS::SRC_FMT;
        }
        if (PNG::XTexLoaderPNG::isValidData(fileData)) {
            return PNG::XTexLoaderPNG::SRC_FMT;
        }
        if (JPG::XTexLoaderJPG::isValidData(fileData)) {
            return JPG::XTexLoaderJPG::SRC_FMT;
        }
        if (PSD::XTexLoaderPSD::isValidData(fileData)) {
            return PSD::XTexLoaderPSD::SRC_FMT;
        }
        if (TGA::XTexLoaderTGA::isValidData(fileData)) {
            return TGA::XTexLoaderTGA::SRC_FMT;
        }

        X_ASSERT_UNREACHABLE();
        return ImgFileFormat::UNKNOWN;
    }

    bool writeSupported(ImgFileFormat::Enum fmt)
    {
        X_ALIGNED_SYMBOL(char buf[MAX_FMT_CLASS_SIZE], 16);
        core::LinearAllocator allocator(buf, buf + sizeof(buf));

        ITextureFmt* pFmt = Allocfmt(&allocator, fmt);

        const bool res = pFmt->canWrite();

        FreeFmt(pFmt);

        return res;
    }

    const char* getExtension(ImgFileFormat::Enum fmt)
    {
        static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");

        switch (fmt) {
            case ImgFileFormat::CI:
                return CI::XTexLoaderCI::EXTENSION;
            case ImgFileFormat::DDS:
                return DDS::XTexLoaderDDS::EXTENSION;
            case ImgFileFormat::PNG:
                return PNG::XTexLoaderPNG::EXTENSION;
            case ImgFileFormat::TGA:
                return TGA::XTexLoaderTGA::EXTENSION;
            case ImgFileFormat::JPG:
                return JPG::XTexLoaderJPG::EXTENSION;
            case ImgFileFormat::PSD:
                return PSD::XTexLoaderPSD::EXTENSION;

            default:
                X_ASSERT_UNREACHABLE();
                return "";
        }
    }

    // ==================================================================

    bool flipVertical(XTextureFile& img, core::MemoryArenaBase* swap)
    {
        if (img.getDepth() > 1) {
            X_ERROR("Img", "Flipping textures with depth is not currently supported");
            return false;
        }

        if (!img.isValid()) {
            X_ERROR("Img", "Failed to flip image, it's not valid");
            return false;
        }

        if (!core::bitUtil::IsPowerOfTwo(img.getHeight()) || !core::bitUtil::IsPowerOfTwo(img.getWidth())) {
            X_ERROR("Img", "Flipping none pow2 images is not currently supported");
            return false;
        }

        switch (img.getFormat()) {
            // any none block format is simple.

            //RGB(A)
            case Texturefmt::R8G8B8:
            case Texturefmt::A8R8G8B8:
            case Texturefmt::R8G8B8A8:
            case Texturefmt::R8G8B8A8_SRGB:
            case Texturefmt::R8G8B8A8_SNORM:
            case Texturefmt::R8G8B8A8_TYPELESS:
            case Texturefmt::R8G8B8A8_SINT:
            case Texturefmt::R8G8B8A8_UINT:

            // BGR(A)
            case Texturefmt::B8G8R8:
            case Texturefmt::B8G8R8A8:
            case Texturefmt::B8G8R8A8_SRGB:
            case Texturefmt::B8G8R8A8_TYPELESS: {
                core::Array<uint8_t> row(swap);

                // just wanna swap rows basically.
                for (size_t i = 0; i < img.getNumFaces(); i++) {
                    uint8_t* pFace = img.getFace(i);
                    size_t rowBytes = img.getLevelRowbytes(i);
                    int32_t numRows = img.getHeight();

                    row.resize(rowBytes);

                    uint8_t* pLowRow = pFace;
                    uint8_t* pHighRow = pFace + ((numRows - 1) * rowBytes);

                    for (; pLowRow < pHighRow; pLowRow += rowBytes, pHighRow -= rowBytes) {
                        std::memcpy(row.data(), pLowRow, rowBytes);
                        std::memcpy(pLowRow, pHighRow, rowBytes);
                        std::memcpy(pHighRow, row.data(), rowBytes);
                    }
                }

            } break;

            default:
                X_ERROR("Img", "Flip not supported for fmt: \"%S\"", Texturefmt::ToString(img.getFormat()));
                return false;
        }

        return true;
    }

    // ==================================================================

    bool bgrToRgb(XTextureFile& img, core::MemoryArenaBase* swap)
    {
        X_UNUSED(swap);

        if (img.getFormat() != Texturefmt::B8G8R8A8 && img.getFormat() != Texturefmt::B8G8R8) {
            X_ERROR("Img", "Source image is not brg, fmt: %s", Texturefmt::ToString(img.getFormat()));
            return false;
        }

        if (img.getDepth() > 1) {
            X_ERROR("Img", "Converting to bgr not supported for textures with depth");
            return false;
        }

        for (size_t face = 0; face < img.getNumFaces(); face++) {
            for (size_t mip = 0; mip < img.getNumMips(); mip++) {
                uint8_t* pSrc = img.getLevel(face, mip);

                if (img.getFormat() == Texturefmt::B8G8R8) {
                    const size_t numPixel = img.getLevelSize(mip) / 3;
                    X_ASSERT(numPixel * 3 == img.getLevelSize(mip), "Pixel num calculation errro")
                    (img.getLevelSize(mip), numPixel);

                    for (size_t i = 0; i < numPixel; i++) {
                        std::swap(pSrc[0], pSrc[2]);
                        pSrc += 3;
                    }
                }
                else if (img.getFormat() == Texturefmt::B8G8R8A8) {
                    const size_t numPixel = img.getLevelSize(mip) / 4;
                    X_ASSERT(numPixel * 4 == img.getLevelSize(mip), "Pixel num calculation errro")
                    (img.getLevelSize(mip), numPixel);

                    for (size_t i = 0; i < numPixel; i++) {
                        std::swap(pSrc[0], pSrc[2]);
                        pSrc += 4;
                    }
                }
                else {
                    X_ASSERT_UNREACHABLE();
                }
            }
        }

        if (img.getFormat() == Texturefmt::B8G8R8A8) {
            img.setFormat(Texturefmt::R8G8B8A8);
        }
        else if (img.getFormat() == Texturefmt::B8G8R8) {
            img.setFormat(Texturefmt::R8G8B8);
        }
        else {
            X_ASSERT_UNREACHABLE();
        }

        return true;
    }

    bool bgrToRgb(const XTextureFile& img, XTextureFile& imgOut, core::MemoryArenaBase* swap)
    {
        X_UNUSED(swap);

        if (img.getFormat() == Texturefmt::B8G8R8A8) {
            imgOut.setFormat(Texturefmt::R8G8B8A8);
        }
        else if (img.getFormat() == Texturefmt::B8G8R8) {
            imgOut.setFormat(Texturefmt::R8G8B8);
        }
        else {
            X_ERROR("Img", "Source image is not brg, fmt: %s", Texturefmt::ToString(img.getFormat()));
            return false;
        }

        if (img.getDepth() > 1) {
            X_ERROR("Img", "Converting to bgr not supported for textures with depth");
            return false;
        }

        imgOut.setSize(img.getSize());
        imgOut.setType(img.getType());
        imgOut.setNumMips(img.getNumMips());
        imgOut.setNumFaces(img.getNumFaces());
        imgOut.setDepth(img.getDepth());
        imgOut.resize();

        for (size_t face = 0; face < img.getNumFaces(); face++) {
            for (size_t mip = 0; mip < img.getNumMips(); mip++) {
                const uint8_t* pSrc = img.getLevel(face, mip);
                uint8_t* pDst = imgOut.getLevel(face, mip);

                if (img.getFormat() == Texturefmt::B8G8R8) {
                    const size_t numPixel = img.getLevelSize(mip) / 3;
                    X_ASSERT(numPixel * 3 == img.getLevelSize(mip), "Pixel num calculation errro")
                    (img.getLevelSize(mip), numPixel);

                    for (size_t i = 0; i < numPixel; i++) {
                        pDst[0] = pSrc[2];
                        pDst[1] = pSrc[1];
                        pDst[2] = pSrc[0];

                        pSrc += 3;
                        pDst += 3;
                    }
                }
                else if (img.getFormat() == Texturefmt::B8G8R8A8) {
                    const size_t numPixel = img.getLevelSize(mip) / 4;
                    X_ASSERT(numPixel * 4 == img.getLevelSize(mip), "Pixel num calculation errro")
                    (img.getLevelSize(mip), numPixel);

                    for (size_t i = 0; i < numPixel; i++) {
                        pDst[0] = pSrc[2];
                        pDst[1] = pSrc[1];
                        pDst[2] = pSrc[0];
                        pDst[3] = pSrc[3];

                        pSrc += 4;
                        pDst += 4;
                    }
                }
                else {
                    X_ASSERT_UNREACHABLE();
                }
            }
        }

        return true;
    }

    // ==================================================================

    bool loadImage(core::MemoryArenaBase* swap, const core::Array<uint8_t>& fileData, ImgFileFormat::Enum fmt, XTextureFile& img)
    {
        core::XFileFixedBuf file(fileData.begin(), fileData.end());

        return loadImage(swap, &file, fmt, img);
    }

    bool loadImage(core::MemoryArenaBase* swap, core::XFile* pFile, ImgFileFormat::Enum fmt, XTextureFile& img)
    {
        X_ALIGNED_SYMBOL(char buf[MAX_FMT_CLASS_SIZE], 16);
        core::LinearAllocator allocator(buf, buf + sizeof(buf));

        ITextureFmt* pFmt = Allocfmt(&allocator, fmt);

        const bool res = pFmt->loadTexture(pFile, img, swap);

        FreeFmt(pFmt);

        return res;
    }

    bool saveImage(core::MemoryArenaBase* swap, core::XFile* pFile, ImgFileFormat::Enum fmt, const XTextureFile& img)
    {
        X_ALIGNED_SYMBOL(char buf[MAX_FMT_CLASS_SIZE], 16);
        core::LinearAllocator allocator(buf, buf + sizeof(buf));

        ITextureFmt* pFmt = Allocfmt(&allocator, fmt);

        const bool res = pFmt->saveTexture(pFile, img, swap);

        FreeFmt(pFmt);

        return res;
    }

} // namespace Util

X_NAMESPACE_END
