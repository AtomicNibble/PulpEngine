#include "stdafx.h"
#include "TextureLoaderPNG.h"

#include <IFileSys.h>

#include <Memory\MemCursor.h>
#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

#include <Containers\Array.h>
#include <Compression\Zlib.h>

#include <Hashing\crc32.h>

#include "TextureFile.h"

X_NAMESPACE_BEGIN(texture)

namespace PNG
{
    namespace
    {
        static const char* PNG_FILE_EXTENSION = ".png";
        static const int64_t PNG_FILE_MAGIC = 0xA1A0A0D474E5089;
        static const int32_t PNG_TAG_IHDR = X_TAG('I', 'H', 'D', 'R');
        static const int32_t PNG_TAG_IDAT = X_TAG('I', 'D', 'A', 'T');
        static const int32_t PNG_TAG_IEND = X_TAG('I', 'E', 'N', 'D');
        static const int32_t PNG_TAG_TEXT = X_TAG('t', 'E', 'X', 't');

        struct PngColorType
        {
            enum Enum : int8_t
            {
                GREYSCALE = 0,
                TRUECOLOR = 2,
                INDEXED = 3,
                GREYSCALE_ALPHA = 4,
                TRUECOLOR_ALPHA = 6
            };
        };

        struct CompressionMethod
        {
            enum Enum : int8_t
            {
                Deflate
            };
        };

        struct FilterMethod
        {
            enum Enum : int8_t
            {
                Adaptivee
            };
        };

        struct FilterMethodType
        {
            enum Enum : int8_t
            {
                None,
                Sub,
                Up,
                Average,
                Paeth
            };
        };

        struct InterlaceMethod
        {
            enum Enum : int8_t
            {
                None,
                Adam7
            };
        };

        X_PACK_PUSH(1)
        struct IHDR
        {
            static const int32_t TAG_ID = PNG_TAG_IHDR;
            static const int32_t TAG_SIZE = 13;

            int32_t tag; // makes crc32 more easy to calculate, can do as single block.
            int32_t width;
            int32_t height;
            int8_t bitDepth;
            PngColorType::Enum colType;
            CompressionMethod::Enum compression;
            FilterMethod::Enum filter;
            InterlaceMethod::Enum interLace;
        };

        struct IDAT
        {
            static const int32_t TAG_ID = PNG_TAG_IDAT;
        };

        struct IEND
        {
            static const int32_t TAG_ID = PNG_TAG_IEND;
            static const int32_t TAG_SIZE = 0;
        };

        struct tEXt
        {
            static const int32_t TAG_ID = PNG_TAG_TEXT;
        };

        X_PACK_POP

        X_ENSURE_SIZE(IHDR, IHDR::TAG_SIZE + 4);

        static uint32_t png_get_bpp(uint32_t depth, PngColorType::Enum color_type)
        {
            uint32_t bpp;
            switch (color_type) {
                case PngColorType::GREYSCALE:
                    bpp = 1;
                    break;
                case PngColorType::TRUECOLOR:
                    bpp = 3;
                    break;
                case PngColorType::INDEXED:
                    bpp = 1;
                    break;
                case PngColorType::GREYSCALE_ALPHA:
                    bpp = 2;
                    break;
                case PngColorType::TRUECOLOR_ALPHA:
                    bpp = 4;
                    break;
                default:
                    X_ASSERT_UNREACHABLE();
                    return 0;
            }
            bpp *= depth / 8;
            return bpp;
        }

        static Texturefmt::Enum map_png_format(PngColorType::Enum color_type)
        {
            switch (color_type) {
                case PngColorType::TRUECOLOR:
                    return Texturefmt::R8G8B8;
                case PngColorType::TRUECOLOR_ALPHA:
                    return Texturefmt::R8G8B8A8;
                default:
                    break;
            }

            X_ASSERT_UNREACHABLE();
            return Texturefmt::UNKNOWN;
        }

        struct Png_Header
        {
            int64_t magic;

            bool isValid() const
            {
                return magic == PNG_FILE_MAGIC;
            }
        };

    } // namespace

    const char* XTexLoaderPNG::EXTENSION = PNG_FILE_EXTENSION;

    XTexLoaderPNG::XTexLoaderPNG()
    {
    }

    XTexLoaderPNG::~XTexLoaderPNG()
    {
    }

    bool XTexLoaderPNG::isValidData(const DataVec& fileData)
    {
        if (fileData.size() < sizeof(Png_Header)) {
            return false;
        }

        const Png_Header* pHdr = reinterpret_cast<const Png_Header*>(fileData.data());

        return pHdr->magic == PNG_FILE_MAGIC;
    }

    // ITextureFmt
    const char* XTexLoaderPNG::getExtension(void) const
    {
        return PNG_FILE_EXTENSION;
    }

    ImgFileFormat::Enum XTexLoaderPNG::getSrcFmt(void) const
    {
        return ImgFileFormat::PNG;
    }

    bool XTexLoaderPNG::canLoadFile(const core::Path<char>& path) const
    {
        return core::strUtil::IsEqual(PNG_FILE_EXTENSION, path.extension());
    }

    bool XTexLoaderPNG::canLoadFile(const DataVec& fileData) const
    {
        return isValidData(fileData);
    }

    bool XTexLoaderPNG::loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_ASSERT_NOT_NULL(file);
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pCore);

        core::Crc32* pCrc = gEnv->pCore->GetCrc32();

        Png_Header hdr;
        if (file->readObj(hdr) != sizeof(hdr)) {
            X_ERROR("TexturePNG", "Failed to read header");
            return false;
        }

        if (!hdr.isValid()) {
            X_ERROR("TexturePNG", "invalid png header.");
            return false;
        }

        // ihdr length
        int32_t length;
        if (file->readObj(length) != sizeof(length)) {
            X_ERROR("TexturePNG", "failed to read TAG length");
            return false;
        }
        length = core::Endian::swap(length);

        if (length != IHDR::TAG_SIZE) {
            X_ERROR("TexturePNG", "invalid png length. provided: %" PRIu32 " required: %" PRIi32, length, IHDR::TAG_SIZE);
            return false;
        }

        IHDR ihdr;
        if (file->readObj(ihdr) != sizeof(ihdr)) {
            X_ERROR("TexturePNG", "Failed to read IHDR data");
            return false;
        }

        if (ihdr.tag != IHDR::TAG_ID) {
            X_ERROR("TexturePNG", "invalid sub tag. expoected: IHDR");
            return false;
        }

        uint32_t tagCrc;
        if (file->readObj(tagCrc) != sizeof(tagCrc)) {
            X_ERROR("TexturePNG", "Failed to read tag crc");
            return false;
        }
        tagCrc = core::Endian::swap(tagCrc);

        // check crc.
        if (pCrc->GetCRC32OfObject(ihdr) != tagCrc) {
            X_ERROR("TexturePNG", "IHDR crc mismatch");
            return false;
        }

        // swap the dims.
        ihdr.width = core::Endian::swap(ihdr.width);
        ihdr.height = core::Endian::swap(ihdr.height);

        if (ihdr.height < 1 || ihdr.height > TEX_MAX_DIMENSIONS || ihdr.width < 1 || ihdr.width > TEX_MAX_DIMENSIONS) {
            X_ERROR("TexturePNG", "invalid image dimensions. provided: %" PRIi32 "x%" PRIi32 " max: %" PRIu32 "x%" PRIu32,
                ihdr.height, ihdr.width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);
            return false;
        }

        if (!core::bitUtil::IsPowerOfTwo(ihdr.height) || !core::bitUtil::IsPowerOfTwo(ihdr.width)) {
            X_ERROR("TexturePNG", "invalid image dimensions, must be power of two. provided: %" PRIi32 "x%" PRIi32,
                ihdr.height, ihdr.width);
            return false;
        }

        if (ihdr.colType == PngColorType::INDEXED) {
            X_ERROR("TexturePNG", "invalid color type. expected: TRUECOLOR | TRUECOLOR_ALPHA");
            return false;
        }

        if (ihdr.bitDepth != 8 && ihdr.bitDepth != 16) {
            X_ERROR("TexturePNG", "invalid depth. provided: %" PRIi8 " expected: 8 | 16", ihdr.bitDepth);
            return false;
        }

        if (ihdr.bitDepth == 16) {
            X_ERROR("TexturePNG", "16 bit channels not currently supported");
            return false;
        }

        if (ihdr.interLace != InterlaceMethod::None) {
            X_ERROR("TexturePNG", "interlace not supported");
            return false;
        }

        // skip all the tags untill we reach PNG_TAG_IDAT
        uint32_t tagName;
        for (int32_t i = 0; i < 256; i++) {
            if (file->readObj(length) != sizeof(length)) {
                X_ERROR("TexturePNG", "failed to read TAG length");
                return false;
            }

            length = core::Endian::swap(length);

            // read the Tag name
            if (file->readObj(tagName) != sizeof(tagName)) {
                X_ERROR("TexturePNG", "Failed to read tag");
                return false;
            }

            if (tagName == PNG_TAG_IDAT) {
                break;
            }

            // skip length + crc
            file->seek(length + 4, core::SeekMode::CUR);
        }

        // we got a PNG_TAG_IDAT?
        if (tagName != PNG_TAG_IDAT) {
            X_ERROR("TexturePNG", "failed to find IDAT tag in file");
            return false;
        }

        // inflated_data is the uncompressed image data.
        // make img info.
        TextureFlags flags;
        flags.Set(TextureFlags::NOMIPS);

        if (ihdr.colType == PngColorType::TRUECOLOR_ALPHA) {
            flags.Set(TextureFlags::ALPHA);
        }

        imgFile.setFormat(map_png_format(ihdr.colType));
        imgFile.setNumFaces(1);
        imgFile.setNumMips(1);
        imgFile.setDepth(1);
        imgFile.setFlags(flags);
        imgFile.setType(TextureType::T2D);
        imgFile.setHeigth(safe_static_cast<uint16_t, uint32_t>(ihdr.height));
        imgFile.setWidth(safe_static_cast<uint16_t, uint32_t>(ihdr.width));
        imgFile.resize();

        // ok so the length is the size of the compreseed block we need to read.
        // it is possible to have multiple IDAT blocks.
        const uint32_t bpp = png_get_bpp(ihdr.bitDepth, ihdr.colType);
        const uint32_t inflated_size = (ihdr.width * ihdr.height * bpp);

        X_ASSERT(inflated_size == imgFile.getFaceSize(), "Calculated size don't match face size")(inflated_size, imgFile.getFaceSize()); 

        if (!LoadChucksIDAT(swapArena, pCrc, file, length, imgFile)) {
            X_ERROR("TexturePNG", "failed to load PNG chunks");
            return false;
        }

        return true;
    }

    bool XTexLoaderPNG::LoadChucksIDAT(core::MemoryArenaBase* swapArea, core::Crc32* pCrc, core::XFile* file,
        int32_t idatBlockLength, XTextureFile& imgFile)
    {
        X_ASSERT_NOT_NULL(file);

        const size_t rowBytes = Util::rowBytes(imgFile.getWidth(), 1, imgFile.getFormat()) + 1;
        size_t bytesLeft = imgFile.getFaceSize();
        uint8_t* pDst = imgFile.getFace(0);

        using namespace core::Compression;

        ZlibInflate inflater(swapArea, [&](const uint8_t* pData, size_t len, size_t inflatedOffset) {
            X_UNUSED(inflatedOffset);
            X_ASSERT(rowBytes == len, "deflated buffer not match row size")(rowBytes, len); 

            // ignore filter byte.
            ++pData;
            --len;

            X_ASSERT(bytesLeft >= len, "Recived too much data")(bytesLeft, len); 
            bytesLeft -= len;

            std::memcpy(pDst, pData, len);
            pDst += len;
        });

        // row bytes is rgb / rgba + 1 filter byte.
        inflater.setBufferSize(rowBytes);

        uint32_t tagName;
        uint32_t blockCrc;

        ZlibInflate::Result::Enum zRes;

        // going to just load the block in chuncks so even if a png is saved as single large block.
        // my memory usage will stay low.
        core::Array<uint8_t> deflatedData(swapArea);
        deflatedData.resize(IO_READ_BLOCK_SIZE);

        do {
#if VALIDATE_IDAT_CRC
            uint32_t calcCrc = pCrc->Begin();
            pCrc->Update(&IDAT::TAG_ID, sizeof(IDAT::TAG_ID), calcCrc);
#endif // !VALIDATE_IDAT_CRC

            int32_t bytesToRead = idatBlockLength;
            while (bytesToRead > 0) {
                const int32_t readSize = core::Min(bytesToRead, IO_READ_BLOCK_SIZE);

                if (file->read(deflatedData.ptr(), readSize) != readSize) {
                    X_ERROR("TexturePNG", "failed to reed block of size: %" PRIuS, readSize);
                    return false;
                }

#if VALIDATE_IDAT_CRC
                pCrc->Update(deflatedData.ptr(), readSize, calcCrc);
#endif // !VALIDATE_IDAT_CRC

                // infalte it baby.
                zRes = inflater.Inflate(deflatedData.ptr(), readSize);

                if (zRes == ZlibInflate::Result::ERROR) {
                    X_ERROR("TexturePNG", "Zlib error");
                    return false;
                }

                bytesToRead -= readSize;
            }

            // after block there is a crc32
            if (file->readObj(blockCrc) != sizeof(blockCrc)) {
                X_ERROR("TexturePNG", "failed to read tag crc");
                return false;
            }
            blockCrc = core::Endian::swap(blockCrc);

#if VALIDATE_IDAT_CRC
            calcCrc = pCrc->Finish(calcCrc);
            if (blockCrc != calcCrc) {
                X_ERROR("TexturePNG", "IDAT crc mismatch. dataCrc: %" PRIu32 " expectedCrc: %" PRIu32, calcCrc, blockCrc);
                return false;
            }
#endif // !VALIDATE_IDAT_CRC

            uint32_t blockLen;
            if (file->readObj(blockLen) != sizeof(blockLen)) {
                X_ERROR("TexturePNG", "failed to read tag length");
                return false;
            }

            blockLen = core::Endian::swap(blockLen);

            // check for next tag
            if (file->readObj(tagName) != sizeof(tagName)) {
                X_ERROR("TexturePNG", "failed to read tag name");
                return false;
            }

            idatBlockLength = blockLen;

        } while (tagName == PNG_TAG_IDAT);

        if (zRes != ZlibInflate::Result::DONE) {
            X_ERROR("TexturePNG", "Potential zlib error, did not inflate expected amount");
            return false;
        }

        if (tagName != PNG_TAG_IEND) {
            X_WARNING("TexturePNG", "failed to find IEND tag");
        }

        // check the image is fully valid
        if (bytesLeft != 0) {
            X_ERROR("TexturePNG", "Not all texture data was populated %" PRIuS " left uninitialized", bytesLeft);
            return false;
        }

        if (file->readObj(blockCrc) != sizeof(blockCrc)) {
            X_WARNING("TexturePNG", "failed to read IEND crc");
            return false;
        }
        blockCrc = core::Endian::swap(blockCrc);

        if (pCrc->GetCRC32OfObject(tagName) != blockCrc) {
            X_ERROR("TexturePNG", "IEND crc mismatch");
            return false;
        }

#if 0 // could just be more tags, so don't warn
		const auto bytesLeft = file->remainingBytes();
		if (bytesLeft != 0) {
			X_WARNING("TexturePNG", "Trailing bytes after IEND");
		}
#endif

        return true;
    }

    bool XTexLoaderPNG::saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        // some validation.
        if (imgFile.getFormat() != Texturefmt::R8G8B8A8 && imgFile.getFormat() != Texturefmt::R8G8B8 && imgFile.getFormat() != Texturefmt::A8) {
            X_ERROR("TexturePNG", "Saving fmt \"%s\" is not supported", Texturefmt::ToString(imgFile.getFormat()));
            return false;
        }
        if (imgFile.getNumMips() > 1) {
            X_ERROR("TexturePNG", "Can't save image with mips");
            return false;
        }
        if (imgFile.getNumFaces() > 1) {
            X_ERROR("TexturePNG", "Can't save image with multiple faces");
            return false;
        }

        core::Crc32* pCrc = gEnv->pCore->GetCrc32();

        PngColorType::Enum colType = PngColorType::TRUECOLOR;

        switch (imgFile.getFormat())
        {
            case Texturefmt::R8G8B8A8:
                colType = PngColorType::TRUECOLOR_ALPHA;
                break;
            case Texturefmt::R8G8B8:
                colType = PngColorType::TRUECOLOR;
                break;
            case Texturefmt::A8:
                colType = PngColorType::GREYSCALE;
                break;
            default:
                X_ASSERT_UNREACHABLE();
                return false;
        }

        file->writeObj(PNG_FILE_MAGIC);

        // write a IHDR
        IHDR ihdr;
        ihdr.tag = IHDR::TAG_ID;
        ihdr.width = core::Endian::swap(imgFile.getWidth());
        ihdr.height = core::Endian::swap(imgFile.getHeight());
        ihdr.bitDepth = 8;                             // bit depth
        ihdr.colType = colType;                        // color type
        ihdr.compression = CompressionMethod::Deflate; // compression method
        ihdr.filter = FilterMethod::Adaptivee;         // filter method
        ihdr.interLace = InterlaceMethod::None;        // interlace method

        file->writeObj(core::Endian::swap<int32_t>(IHDR::TAG_SIZE));
        file->writeObj(ihdr);
        file->writeObj(core::Endian::swap(pCrc->GetCRC32OfObject(ihdr)));

        // complete my ego.
        {
            core::StackString<128> str;
            str.appendFmt("%s - %s - %s - %s", X_ENGINE_NAME, X_ENGINE_VERSION_STR, X_CPUSTRING, X_PLATFORM_STR);

            uint32_t crc = pCrc->Begin();
            pCrc->Update(&tEXt::TAG_ID, sizeof(tEXt::TAG_ID), crc);
            pCrc->Update(str.c_str(), str.length(), crc);
            crc = pCrc->Finish(crc);

            file->writeObj(core::Endian::swap<int32_t>(static_cast<int32_t>(str.length())));
            file->writeObj(tEXt::TAG_ID);
            file->write(str.c_str(), str.length());
            file->writeObj(core::Endian::swap(crc));
        }

        // now we write the data.
        const int32_t srcSize = safe_static_cast<int32_t>(imgFile.getFaceSize());
        const uint8_t* const pSrc = imgFile.getFace(0);
        const uint8_t* pSrcCur = pSrc;

        using namespace core::Compression;

        // micro optermisation, don't bother calculating tag crc each time lol.
        uint32_t idataCrc = pCrc->Begin();
        pCrc->Update(&IDAT::TAG_ID, sizeof(IDAT::TAG_ID), idataCrc);

        ZlibDefalte zlib(swapArena, [&](const uint8_t* pData, size_t len, size_t deflateOffset) {
            X_UNUSED(deflateOffset);
            // get crc of block.
            uint32_t crc = idataCrc;
            pCrc->Update(pData, len, crc);
            crc = pCrc->Finish(crc);

            // write it.
            file->writeObj(core::Endian::swap<int32_t>(static_cast<int32_t>(len)));
            file->writeObj(IDAT::TAG_ID);
            file->write(pData, len);
            file->writeObj(core::Endian::swap(crc));
        });

        zlib.setBufferSize(BLOCK_SIZE);

        // right png has fucking lost it's lemons.
        // and requires a filter byte at start of each row.
        const int32_t rowBytes = Util::dataSize(imgFile.getWidth(), 1, imgFile.getFormat());
        const int32_t rows = imgFile.getHeight();
        for (int32_t row = 0; row < rows; row++) {
            // stupid filter byte.
            const FilterMethodType::Enum filterType = FilterMethodType::None;

            auto res = zlib.Deflate(&filterType, sizeof(filterType), false);
            if (res != ZlibDefalte::Result::OK) {
                X_ERROR("TexturePNG", "Failed to deflate image \"%s\"", ZlibDefalte::Result::ToString(res));
                return false;
            }

            // deflate row.
            const bool lastRow = (row + 1) == rows;
            res = zlib.Deflate(pSrcCur, rowBytes, lastRow);

            if (lastRow) {
                if (res != ZlibDefalte::Result::DONE) {
                    X_ERROR("TexturePNG", "Failed to deflate(flush) image \"%s\"", ZlibDefalte::Result::ToString(res));
                    return false;
                }
            }
            else {
                if (res != ZlibDefalte::Result::OK) {
                    X_ERROR("TexturePNG", "Failed to deflate image \"%s\"", ZlibDefalte::Result::ToString(res));
                    return false;
                }
            }

            pSrcCur += rowBytes;

            X_ASSERT(pSrcCur <= (pSrc + srcSize), "Out of range")(pSrcCur, pSrc, srcSize); 
        }

        file->writeObj(core::Endian::swap<int32_t>(IEND::TAG_SIZE));
        file->writeObj(IEND::TAG_ID);
        file->writeObj(core::Endian::swap(pCrc->GetCRC32OfObject(IEND::TAG_ID)));
        return true;
    }

    // ~ITextureFmt

} // namespace PNG

X_NAMESPACE_END