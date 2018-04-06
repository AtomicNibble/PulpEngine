#include "stdafx.h"
#include "TextureLoaderPSD.h"

#include <Util\EndianUtil.h>
#include <IFileSys.h>

#include "TextureFile.h"

X_NAMESPACE_BEGIN(texture)

#ifdef RGB
#undef RGB
#endif // !RGB

#ifdef CMYK
#undef CMYK
#endif // !CMYK

namespace PSD
{
    namespace
    {
        static const char* PSD_FILE_EXTENSION = ".psd";
        static const uint32 PSD_FILE_FOURCC = X_TAG('S', 'P', 'B', '8'); // reser cus it's it's got edian

        X_DECLARE_ENUM(PsdColorMode)
        (
            Bitmap,
            GrayScale,
            Indexed,
            RGB,
            CMYK,
            MultiChannel,
            DuoTone,
            LabColor);

        static_assert(PsdColorMode::RGB == 3, "Rgb must be 3");

        X_PRAGMA(pack(push, 1))
        struct PsdHeader
        {
            uint32 fourCC;     // Always equal to 8BPS.
            uint16 version;    // Always equal to 1
            uint8 reserved[6]; // Must be zero
            uint16 channels;   // Number of any channels inc. alphas
            uint32 height;     // Rows Height of image in pixel
            uint32 width;      // Colums Width of image in pixel
            uint16 depth;      // Bits/channel
            uint16 mode;       // Color mode of the file (Bitmap/Grayscale..)

            void EdianFlip(void)
            {
                fourCC = core::Endian::swap(fourCC);
                version = core::Endian::swap(version);
                channels = core::Endian::swap(channels);
                height = core::Endian::swap(height);
                width = core::Endian::swap(width);
                depth = core::Endian::swap(depth);
                mode = core::Endian::swap(mode);
            }

            X_INLINE bool isValid(void) const
            {
                return fourCC == PSD_FILE_FOURCC;
            }
        };
        X_PRAGMA(pack(pop))

        uint16 getShiftFromChannel(uint8 channelNr, const PsdHeader& header)
        {
            switch (channelNr) {
                case 0:
                    return 16; // red
                case 1:
                    return 8; // green
                case 2:
                    return 0; // blue
                case 3:
                    if (header.channels == 4)
                        return 24;
                    X_ERROR("TexturePSD", "Unknown channel count for channel number 3. count: %i", header.channels);
                case 4:
                    return 24; // alpha
                default:
                    X_ERROR("TexturePSD", "Unknown channel number: %i", channelNr);
                    break;
            }

            X_ASSERT_UNREACHABLE();
            return 0;
        }

        bool readRLEImageData(core::MemoryArenaBase* swapArena, core::XFile* file, const PsdHeader& header, uint32_t* imageData)
        {
            /*	If the compression code is 1, the image data
			starts with the byte counts for all the scan lines in the channel
			(LayerBottom LayerTop), with each count stored as a two
			byte value. The RLE compressed data follows, with each scan line
			compressed separately. The RLE compression is the same compres-sion
			algorithm used by the Macintosh ROM routine PackBits, and
			the TIFF standard.
			If the Layer's Size, and therefore the data, is odd, a pad byte will
			be inserted at the end of the row.
			*/

            /*
			A pseudo code fragment to unpack might look like this:

			Loop until you get the number of unpacked bytes you are expecting:
			Read the next source byte into n.
			If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
			Else if n is between -127 and -1 inclusive, copy the next byte -n+1
			times.
			Else if n is -128, noop.
			Endloop

			In the inverse routine, it is best to encode a 2-byte repeat run as a replicate run
			except when preceded and followed by a literal run. In that case, it is best to merge
			the three runs into one literal run. Always encode 3-byte repeats as replicate runs.
			That is the essence of the algorithm. Here are some additional rules:
			- Pack each row separately. Do not compress across row boundaries.
			- The number of uncompressed bytes per row is defined to be (ImageWidth + 7)
			/ 8. If the uncompressed bitmap is required to have an even number of bytes per
			row, decompress into word-aligned buffers.
			- If a run is larger than 128 bytes, encode the remainder of the run as one or more
			additional replicate runs.
			When PackBits data is decompressed, the result should be interpreted as per com-pression
			type 1 (no compression).
			*/

            core::Array<int8_t> tmpData(swapArena, header.width * header.height);
            core::Array<uint16_t> rleCount(swapArena, header.width * header.channels);

            uint32_t size = 0;

            for (uint32_t y = 0; y < header.height * header.channels; ++y) {
                if (!file->read(&rleCount[y], sizeof(int16_t))) {
                    X_ERROR("TexturePSD", "failed to read rle rows");
                    return false;
                }

#ifndef __BIG_ENDIAN__
                // rleCount[y] = os::Byteswap::byteswap(rleCount[y]);
                rleCount[y] = core::Endian::swap(rleCount[y]);
#endif
                size += rleCount[y];
            }

            core::Array<int8_t> buf(swapArena, size);

            if (!file->read(buf.ptr(), size)) {
                X_ERROR("TexturePSD", "failed to read rle rows");
                return false;
            }

            uint16_t* rcount = rleCount.ptr();

            int8_t rh;
            uint16_t bytesRead;
            int8_t* dest;
            int8_t* pBuf = buf.ptr();

            // decompress packbit rle

            for (uint32_t channel = 0; channel < header.channels; channel++) {
                for (uint32_t y = 0; y < header.height; ++y, ++rcount) {
                    bytesRead = 0;
                    dest = &tmpData[y * header.width];

                    while (bytesRead < *rcount) {
                        rh = *pBuf++;
                        ++bytesRead;

                        if (rh >= 0) {
                            ++rh;

                            while (rh--) {
                                *dest = *pBuf++;
                                ++bytesRead;
                                ++dest;
                            }
                        }
                        else if (rh > -128) {
                            rh = -rh + 1;

                            while (rh--) {
                                *dest = *pBuf;
                                ++dest;
                            }

                            ++pBuf;
                            ++bytesRead;
                        }
                    }
                }

                uint16_t shift = getShiftFromChannel(static_cast<uint8_t>(channel), header);

                if (shift != 0) {
                    uint32_t mask = 0xffu << static_cast<uint32_t>(shift);

                    for (uint32_t x = 0; x < header.width; ++x) {
                        for (uint32_t y = 0; y < header.height; ++y) {
                            uint32_t index = x + y * header.width;
                            imageData[index] = ~(~imageData[index] | mask);
                            imageData[index] |= tmpData[index] << shift;
                        }
                    }
                }
            }

            return true;
        }

        bool readRawImageData(core::MemoryArenaBase* swapArena, core::XFile* file, const PsdHeader& header, uint32_t* imageData)
        {
            core::Array<uint8_t> tmpData(swapArena, header.width * header.height);

            for (int32_t channel = 0; channel < header.channels && channel < 3; ++channel) {
                if (!file->read(tmpData.ptr(), sizeof(uint8_t) * header.width * header.height)) {
                    X_ERROR("TexturePSD", "failed to read color channel");
                    break;
                }

                uint16_t shift = getShiftFromChannel(static_cast<uint8_t>(channel), header);
                if (shift != 0) {
                    uint32_t mask = 0xffu << static_cast<uint32_t>(shift);

                    for (uint32_t x = 0; x < header.width; ++x) {
                        for (uint32_t y = 0; y < header.height; ++y) {
                            uint32_t index = x + y * header.width;
                            imageData[index] = ~(~imageData[index] | mask);
                            imageData[index] |= tmpData[index] << shift;
                        }
                    }
                }
            }

            return true;
        }

    } // namespace

    const char* XTexLoaderPSD::EXTENSION = PSD_FILE_EXTENSION;

    XTexLoaderPSD::XTexLoaderPSD()
    {
    }

    XTexLoaderPSD::~XTexLoaderPSD()
    {
    }

    bool XTexLoaderPSD::isValidData(const DataVec& fileData)
    {
        if (fileData.size() < sizeof(PsdHeader)) {
            return false;
        }

        const PsdHeader* pHdr = reinterpret_cast<const PsdHeader*>(fileData.data());

        return pHdr->fourCC == PSD_FILE_FOURCC;
    }

    // ITextureFmt
    const char* XTexLoaderPSD::getExtension(void) const
    {
        return PSD_FILE_EXTENSION;
    }

    ImgFileFormat::Enum XTexLoaderPSD::getSrcFmt(void) const
    {
        return ImgFileFormat::PSD;
    }

    bool XTexLoaderPSD::canLoadFile(const core::Path<char>& path) const
    {
        return core::strUtil::IsEqual(PSD_FILE_EXTENSION, path.extension());
    }

    bool XTexLoaderPSD::canLoadFile(const DataVec& fileData) const
    {
        return isValidData(fileData);
    }

    bool XTexLoaderPSD::loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_ASSERT_NOT_NULL(file);

        uint32_t Length, i;
        uint16_t CompressionType;

        PsdHeader hdr;
        file->readObj(hdr);

        hdr.EdianFlip();

        if (!hdr.isValid()) {
            X_ERROR("TexturePSD", "PSD file is not valid");
            return false;
        }

        if (hdr.version != 1) {
            X_ERROR("TexturePSD", "PSD file version must be 1");
            return false;
        }

        if (hdr.mode != PsdColorMode::RGB) {
            X_ERROR("TexturePSD", "PSD color mode is not supported. providied: %i required: 3", hdr.mode);
            return false;
        }

        if (hdr.depth != 8) {
            X_ERROR("TexturePSD", "PSD depth is not supported. providied: %i required: 8", hdr.depth);
            return false;
        }

        if (hdr.height < 1 || hdr.height > TEX_MAX_DIMENSIONS || hdr.width < 1 || hdr.width > TEX_MAX_DIMENSIONS) {
            X_ERROR("TexturePSD", "invalid image dimensions. provided: %ix%i max: %ix%i", hdr.height, hdr.width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);
            return false;
        }

        if (!core::bitUtil::IsPowerOfTwo(hdr.height) || !core::bitUtil::IsPowerOfTwo(hdr.width)) {
            X_ERROR("TexturePSD", "invalid image dimensions, must be power of two. provided: %ix%i", hdr.height, hdr.width);
            return false;
        }

        // Skip color mode / image resources / layer and mask
        for (i = 0; i < 3; i++) {
            file->readObj(Length);
            Length = core::Endian::swap(Length);
            file->seek(Length, core::SeekMode::CUR);
        }

        file->readObj(CompressionType);
        CompressionType = core::Endian::swap(CompressionType);

        if (CompressionType != 1 && CompressionType != 0) {
            X_ERROR("TexturePSD", "PSD compression mode is not supported. provided: %i required: 1 | 2", CompressionType);
            return false;
        }

        TextureFlags flags;
        flags.Set(TextureFlags::NOMIPS);
        flags.Set(TextureFlags::ALPHA);

        imgFile.setType(TextureType::T2D);
        imgFile.setFormat(Texturefmt::A8R8G8B8);
        imgFile.setHeigth(safe_static_cast<uint16_t, uint32_t>(hdr.height));
        imgFile.setWidth(safe_static_cast<uint16_t, uint32_t>(hdr.width));
        imgFile.setDepth(1);
        imgFile.setNumMips(1);
        imgFile.setNumFaces(1);
        imgFile.setFlags(flags);
        imgFile.resize();

        bool res = false;

        // load the image data.
        if (CompressionType == 1) {
            res = readRLEImageData(swapArena, file, hdr, union_cast<uint32_t*, uint8_t*>(imgFile.getFace(0))); // RLE compressed data
        }
        else {
            res = readRawImageData(swapArena, file, hdr, union_cast<uint32_t*, uint8_t*>(imgFile.getFace(0))); // RAW image data
        }

#if X_DEBUG == 1
        uint64_t left = file->remainingBytes();
        X_WARNING_IF(left > 0, "TexturePSD", "potential read fail, bytes left in file: %i", left);
#endif

        if (!res) {
            return false;
        }

        return true;
    }

    // ~ITextureFmt

} // namespace PSD

X_NAMESPACE_END