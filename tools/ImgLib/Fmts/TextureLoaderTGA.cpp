#include "stdafx.h"
#include "TextureLoaderTGA.h"

#include <IFileSys.h>

#include "TextureFile.h"
#include "Util\TextureUtil.h"

X_NAMESPACE_BEGIN(texture)

namespace TGA
{
    namespace
    {
        static const char* TGA_FILE_EXTENSION = ".tga";

        X_PACK_PUSH(1)
        struct Tga_Header
        {
            uint8_t IDLength;        /* 00h  Size of Image ID field */
            uint8_t ColorMapType;    /* 01h  Color map type */
            uint8_t ImageType;       /* 02h  Image type code */
            uint16_t CMapStart;       /* 03h  Color map origin */
            uint16_t CMapLength;      /* 05h  Color map length */
            uint8_t CMapDepth;       /* 07h  Depth of color map entries */
            uint16_t XOffset;         /* 08h  X origin of image */
            uint16_t YOffset;         /* 0Ah  Y origin of image */
            uint16_t Width;           /* 0Ch  Width of image */
            uint16_t Height;          /* 0Eh  Height of image */
            uint8_t PixelDepth;      /* 10h  Image pixel size */
            uint8_t ImageDescriptor; /* 11h  Image descriptor byte */
        };

        struct Tga_Footer
        {
            X_INLINE bool isValid(void) const
            {
                return std::memcmp(signature, "TRUEVISION-XFILE", sizeof(signature)) == 0;
            }

            uint32_t extensionOffset;
            uint32_t developerAreaOffset;
            uint8_t signature[16];
            uint8_t dot;
            uint8_t null;
        };

        X_ENSURE_SIZE(Tga_Header, 18)
        X_ENSURE_SIZE(Tga_Footer, 26)

        X_PACK_POP

        struct ImageType
        {
            enum Enum
            {
                COLORMAP = 1,
                BGR = 2,
                MONO = 3,
                // run length enc
                COLORMAP_RLE = 9,
                BGR_RLE = 10,
                MONO_RLE = 11
            };
        };

    } // namespace

    const char* XTexLoaderTGA::EXTENSION = TGA_FILE_EXTENSION;

    XTexLoaderTGA::XTexLoaderTGA()
    {
    }

    XTexLoaderTGA::~XTexLoaderTGA()
    {
    }

    bool XTexLoaderTGA::isValidData(const DataVec& fileData)
    {
        if (fileData.size() < sizeof(Tga_Header)) {
            return false;
        }

        Tga_Header hdr = *reinterpret_cast<const Tga_Header*>(fileData.data());

        if (hdr.ColorMapType != 0 && hdr.ColorMapType != 1) {
            return false;
        }
        if (!isValidImageType(hdr.ImageType)) {
            return false;
        }
        if (!(hdr.PixelDepth == 8 || hdr.PixelDepth == 24 || hdr.PixelDepth == 32)) {
            return false;
        }

        return true;
    }

    // ITextureFmt
    const char* XTexLoaderTGA::getExtension(void) const
    {
        return TGA_FILE_EXTENSION;
    }

    ImgFileFormat::Enum XTexLoaderTGA::getSrcFmt(void) const
    {
        return ImgFileFormat::TGA;
    }

    bool XTexLoaderTGA::canLoadFile(const core::Path<char>& path) const
    {
        return core::strUtil::IsEqual(TGA_FILE_EXTENSION, path.extension());
    }

    bool XTexLoaderTGA::canLoadFile(const DataVec& fileData) const
    {
        return isValidData(fileData);
    }

    bool XTexLoaderTGA::loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_ASSERT_NOT_NULL(file);
        X_UNUSED(swapArena);

        Tga_Header hdr;
        if (file->readObj(hdr) != sizeof(hdr)) {
            X_ERROR("Tga", "Failed to read header");
            return false;
        }
    
        // Validate TGA header (is this a TGA file?)
        if (hdr.ColorMapType != 0 && hdr.ColorMapType != 1) {
            X_ERROR("TextureTGA", "invalid color map type. provided: %i expected: 0 | 1", hdr.ColorMapType);
            return false;
        }

        if (!isValidImageType(hdr.ImageType)) {
            X_ERROR("TextureTGA", "invalid image type. provided: %i expected: 1-3 | 9-11", hdr.ImageType);
            return false;
        }

        if (!isBGR(hdr.ImageType)) {
            X_ERROR("TextureTGA", "invalid image type. only rgb maps allowed", hdr.ImageType);
            return false;
        }

        if (isRightToLeft(hdr.ImageDescriptor)) {
            X_ERROR("TextureTGA", "right to left images are not supported", hdr.ImageType);
            return false;
        }

        if (isTopToBottom(hdr.ImageDescriptor)) {
            X_ERROR("TextureTGA", "top to bottom images are not supported", hdr.ImageType);
            return false;
        }

        if (!(hdr.PixelDepth == 8 || hdr.PixelDepth == 24 || hdr.PixelDepth == 32)) {
            X_ERROR("TextureTGA", "invalid pixeldepth. provided: %i expected: 8 | 24 | 32", hdr.PixelDepth);
            return false;
        }

        if (hdr.Height < 1 || hdr.Height > TEX_MAX_DIMENSIONS || hdr.Width < 1 || hdr.Width > TEX_MAX_DIMENSIONS) {
            X_ERROR("TextureTGA", "invalid image dimensions. provided: %ix%i max: %ix%i", hdr.Height, hdr.Width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);
            return false;
        }

        if (!core::bitUtil::IsPowerOfTwo(hdr.Height) || !core::bitUtil::IsPowerOfTwo(hdr.Width)) {
            X_ERROR("TextureTGA", "invalid image dimensions, must be power of two. provided: %ix%i", hdr.Height, hdr.Width);
            return false;
        }

        // load the data.
        TextureFlags flags;
        flags.Set(TextureFlags::NOMIPS);
        flags.Set(TextureFlags::ALPHA);

        // tga is bottom up unless bit is set.
        const bool isFlipped = (hdr.ImageDescriptor & 0x20) == 0;

        uint32_t DataSize = hdr.Width * hdr.Height * (hdr.PixelDepth / 8);

        imgFile.setWidth(safe_static_cast<uint16_t, uint32_t>(hdr.Width));
        imgFile.setHeigth(safe_static_cast<uint16_t, uint32_t>(hdr.Height));
        imgFile.setNumFaces(1);
        imgFile.setDepth(1);
        imgFile.setNumMips(1);
        imgFile.setType(TextureType::T2D);
        // allocate memory for all faces / mips.

        switch (hdr.PixelDepth) {
            case 8:
                imgFile.setFormat(Texturefmt::A8);
                break;
            case 24:
                imgFile.setFormat(Texturefmt::B8G8R8);
                break;
            case 32:
                imgFile.setFormat(Texturefmt::B8G8R8A8);
                break;
            default:
                X_ASSERT_NOT_IMPLEMENTED();
                return false;
        }

        imgFile.resize();

        // read the image data.
        if (isRle(hdr.ImageType)) {
            //some goaty shit.
            uint8_t bpp = safe_static_cast<uint8_t, uint32_t>(hdr.PixelDepth / 8); // bytes per pixel
            uint32_t loaded = 0;
            uint32_t expected = hdr.Width * hdr.Height;

            if (bpp > 4) {
                X_ERROR("TextureTGA", "Invalid bpp for rle, max 4. got: %i", static_cast<int32_t>(bpp));
                return false;
            }

            uint8_t* pCur = imgFile.getFace(0);
            uint8_t* pEnd = pCur + DataSize;

            while (loaded < expected) {
                uint8_t b;
                if (file->readObj(b) != sizeof(b)) {
                    X_ERROR("TextureTGA", "Failed to read rle header byte");
                    return false;
                }

                uint8_t num = (b & ~BIT(7)) + 1;

                if (core::bitUtil::IsBitSet(b, 7)) {
                    // rle
                    uint8_t tmp[4];

                    if (file->read(tmp, bpp) != bpp) {
                        X_ERROR("TextureTGA", "failed to read rle bytes");
                        return false;
                    }

                    for (uint8_t i = 0; i < num; i++) {
                        ++loaded;
                        if (loaded > expected) {
                            // too many
                            X_ERROR("TextureTGA", "Raw packet is too big");
                            return false;
                        }

                        std::memcpy(pCur, tmp, bpp);
                        pCur += bpp;
                    }
                }
                else {
                    if (loaded + num > expected) {
                        // too many
                        X_ERROR("TextureTGA", "Raw packet is too big");
                        return false;
                    }

                    const uint32_t readSize = num * bpp;
                    if (file->read(pCur, readSize) != readSize) {
                        X_ERROR("TextureTGA", "Failed to read raw packet");
                        return false;
                    }

                    loaded += num;
                    pCur += readSize;
                }
            }

            // check all data is valid for the face.
            if (pCur != pEnd) {
                const size_t bytesLeft = pEnd - pCur;
                X_ERROR("TextureTGA", "Failed to correctly read rle incoded image. bytes left: %" PRIuS, bytesLeft);
                return false;
            }
        }
        else {
            size_t bytesRead = file->read(imgFile.getFace(0), DataSize);

            if (bytesRead != DataSize) {
                X_ERROR("TextureTGA", "failed to read image data from. requested: %i bytes recivied: %i bytes", DataSize, bytesRead);
                return false;
            }
        }

        uint64_t left = file->remainingBytes();
        if (left == sizeof(Tga_Footer)) {
            Tga_Footer footer;
            if (file->readObj(footer) != sizeof(footer)) {
                X_WARNING("TextureTGA", "Failed to read potentially footer");
            }
            else {
                if (!footer.isValid()) {
                    X_WARNING("TextureTGA", "Invalid footer data");
                }
            }

#if X_DEBUG == 1
            // update left.
            left = file->remainingBytes();
#endif // !X_DEBUG
        }

#if X_DEBUG == 1
        X_WARNING_IF(left > 0, "TextureTGA", "potential read fail, bytes left in file: %i", left);
#endif // !X_DEBUG

        // flip it
        if (isFlipped) {
            if (!imgFile.flipVertical(swapArena)) {
                X_WARNING("TextureTGA", "Failed to flip texture");
                return false;
            }
        }

        return true;
    }

    bool XTexLoaderTGA::canWrite(void) const
    {
        return true;
    }

    bool XTexLoaderTGA::saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_UNUSED(file, imgFile, swapArena);

        if (imgFile.getNumMips() > 1 || imgFile.getNumFaces() > 1) {
            X_ERROR("Tga", "Can't save image with mips or faces");
            return false;
        }

        Tga_Header hdr;
        core::zero_object(hdr);

        hdr.IDLength = 0;    
        hdr.ColorMapType = 0;

        switch (imgFile.getFormat())
        {
            // case Texturefmt::A8:
            case Texturefmt::B8G8R8:
            case Texturefmt::R8G8B8:
                hdr.ImageType = ImageType::BGR;
                hdr.PixelDepth = 24;
                break;
            case Texturefmt::B8G8R8A8:
                hdr.ImageType = ImageType::BGR;
                hdr.PixelDepth = 32;
                break;

            default:
                X_ERROR("Tga", "Unsupported texture format: %s", Texturefmt::ToString(imgFile.getFormat()));
                return false;
        }

        hdr.CMapStart = 0;
        hdr.CMapLength = 0;
        hdr.CMapDepth = 0;
        hdr.XOffset = 0;
        hdr.YOffset = 0;
        hdr.Width = imgFile.getWidth();
        hdr.Height = imgFile.getHeight();
        hdr.ImageDescriptor = 0x20;

        if (file->writeObj(hdr) != sizeof(hdr)) {
            X_ERROR("Tga", "Failed to write header");
            return false;
        }

        uint32_t dataSize = hdr.Width * hdr.Height * (hdr.PixelDepth / 8);
        X_ASSERT(dataSize == imgFile.getFaceSize(), "Size missmatch")(dataSize, imgFile.getFaceSize());

        if (Util::isBGR(imgFile.getFormat()))
        {
            size_t bytesWrite = file->write(imgFile.getFace(0), dataSize);
            if (bytesWrite != dataSize) {
                X_ERROR("TextureTGA", "Failed to write image data. %i of %i bytes written", dataSize, bytesWrite);
                return false;
            }
        }
        else
        {
            auto rowBytes = Util::rowBytes(imgFile.getWidth(), imgFile.getHeight(), imgFile.getFormat());

            X_ASSERT(rowBytes * imgFile.getHeight() == dataSize, "Size missmatch")(rowBytes * imgFile.getHeight(), dataSize);

            core::Array<uint8_t> row(swapArena, rowBytes);

            const auto* pFace = imgFile.getFace(0);
            const auto* pFaceEnd = pFace + imgFile.getFaceSize();

            for (int32_t i = 0; i < imgFile.getHeight(); i++)
            {
                // build the row.
                auto* pSrcRow = pFace + (i * rowBytes);

                // need to basically flip every 3rd byte.
                // copy the row then flip.
                std::memcpy(row.data(), pSrcRow, rowBytes);

                int32_t rowPixels = imgFile.getWidth();
                X_ASSERT(rowPixels * 3 == row.size(), "Unexpected row size")(rowPixels * 3 == row.size());

                for (int32_t p = 0; p < rowPixels; p++)
                {
                    std::swap(row[(p * 3)], row[(p * 3) + 2]);
                }

                size_t bytesWrite = file->write(row.data(), row.size());
                if (bytesWrite != rowBytes) {
                    X_ERROR("TextureTGA", "Failed to write image row. %i of %i bytes written", dataSize, bytesWrite);
                    return false;
                }
            }
        }
      
        return false;
    }


    // ~ITextureFmt

    bool XTexLoaderTGA::isValidImageType(uint32_t type)
    {
        switch (type) {
            case ImageType::COLORMAP:
            case ImageType::COLORMAP_RLE:
            case ImageType::BGR:
            case ImageType::BGR_RLE:
            case ImageType::MONO:
            case ImageType::MONO_RLE:
                return true;

            default:
                break;
        }
        return false;
    }

    bool XTexLoaderTGA::isColorMap(uint32_t type)
    {
        X_ASSERT(isValidImageType(type), "Invalid format passed")(); 

        switch (type) {
            case ImageType::COLORMAP:
            case ImageType::COLORMAP_RLE:
                return true;

            default:
                break;
        }
        return false;
    }

    bool XTexLoaderTGA::isBGR(uint32_t type)
    {
        X_ASSERT(isValidImageType(type), "Invalid format passed")(); 

        switch (type) {
            case ImageType::BGR:
            case ImageType::BGR_RLE:
                return true;

            default:
                break;
        }
        return false;
    }

    bool XTexLoaderTGA::isMono(uint32_t type)
    {
        X_ASSERT(isValidImageType(type), "Invalid format passed")(); 

        switch (type) {
            case ImageType::MONO:
            case ImageType::MONO_RLE:
                return true;

            default:
                break;
        }
        return false;
    }

    bool XTexLoaderTGA::isRle(uint32_t type)
    {
        X_ASSERT(isValidImageType(type), "Invalid format passed")(); 

        switch (type) {
            case ImageType::COLORMAP_RLE:
            case ImageType::BGR_RLE:
            case ImageType::MONO_RLE:
                return true;

            default:
                break;
        }
        return false;
    }

    bool XTexLoaderTGA::isRightToLeft(uint32_t descriptor)
    {
        return core::bitUtil::IsBitSet(descriptor, 4);
    }

    bool XTexLoaderTGA::isTopToBottom(uint32_t descriptor)
    {
        return core::bitUtil::IsBitSet(descriptor, 5);
    }

} // namespace TGA

X_NAMESPACE_END