#include "stdafx.h"
#include "TextureLoaderDDS.h"

#include <IFileSys.h>

#include "TextureFile.h"

X_NAMESPACE_BEGIN(texture)

X_DISABLE_WARNING(4061)
X_DISABLE_WARNING(4062)

namespace DDS
{
    namespace
    {
        static const char* DDS_FILE_EXTENSION = ".dds";
        #define PIXEL_FMT_FOURCC(a, b, c, d) ((a) | ((b) << 8U) | ((c) << 16U) | ((d) << 24U))

        enum pixel_format
        {
            PIXEL_FMT_INVALID = 0,

            PIXEL_FMT_R16G16 = 0x70,
            PIXEL_FMT_A16B16G16R16F = 0x71,

            PIXEL_FMT_DXT1 = PIXEL_FMT_FOURCC('D', 'X', 'T', '1'),
            PIXEL_FMT_DXT2 = PIXEL_FMT_FOURCC('D', 'X', 'T', '2'),
            PIXEL_FMT_DXT3 = PIXEL_FMT_FOURCC('D', 'X', 'T', '3'),
            PIXEL_FMT_DXT4 = PIXEL_FMT_FOURCC('D', 'X', 'T', '4'),
            PIXEL_FMT_DXT5 = PIXEL_FMT_FOURCC('D', 'X', 'T', '5'),
            PIXEL_FMT_3DC = PIXEL_FMT_FOURCC('A', 'T', 'I', '2'),   // DXN_YX
            PIXEL_FMT_DXN = PIXEL_FMT_FOURCC('A', '2', 'X', 'Y'),   // DXN_XY
            PIXEL_FMT_DXT5A = PIXEL_FMT_FOURCC('A', 'T', 'I', '1'), // ATI1N, http://developer.amd.com/media/gpu_assets/Radeon_X1x00_Programming_Guide.pdf

            // Non-standard, crnlib-specific pixel formats (some of these are supported by ATI's compressonator)
            PIXEL_FMT_DXT5_CCxY = PIXEL_FMT_FOURCC('C', 'C', 'x', 'Y'),
            PIXEL_FMT_DXT5_xGxR = PIXEL_FMT_FOURCC('x', 'G', 'x', 'R'),
            PIXEL_FMT_DXT5_xGBR = PIXEL_FMT_FOURCC('x', 'G', 'B', 'R'),
            PIXEL_FMT_DXT5_AGBR = PIXEL_FMT_FOURCC('A', 'G', 'B', 'R'),

            PIXEL_FMT_DXT1A = PIXEL_FMT_FOURCC('D', 'X', '1', 'A'),

            PIXEL_FMT_R8G8B8 = PIXEL_FMT_FOURCC('R', 'G', 'B', 'x'),
            PIXEL_FMT_L8 = PIXEL_FMT_FOURCC('L', 'x', 'x', 'x'),
            PIXEL_FMT_A8 = PIXEL_FMT_FOURCC('x', 'x', 'x', 'A'),
            PIXEL_FMT_A8L8 = PIXEL_FMT_FOURCC('L', 'x', 'x', 'A'),
            PIXEL_FMT_R8G8B8A8 = PIXEL_FMT_FOURCC('R', 'G', 'B', 'A'),

            PIXEL_FMT_DX10_HEADER = PIXEL_FMT_FOURCC('D', 'X', '1', '0'),
        };

        // Dx10 Formats
        struct dxgiFormat
        {
            enum Enum
            {
                BC6H_TYPELESS = 94,
                BC6H_UF16 = 95,
                BC6H_SF16 = 96,

                BC7_TYPELESS = 97,
                BC7_UNORM = 98,
                BC7_UNORM_SRGB = 99,
            };
        };

        enum dxt_format
        {
            DXTInvalid = -1,

            // DXT1/1A must appear first!
            DXT1,
            DXT1A,

            DXT3,
            DXT5,
            DXT5A,

            DXN_XY, // inverted relative to standard ATI2, 360's DXN
            DXN_YX, // standard ATI2

            BC6_TYPELESS,
            BC6_UF16,
            BC6_SF16,

            BC7_TYPELESS,
            BC7_UNORM,
            BC7_UNORM_SRGB,
        };

        const unsigned DDSMaxImageDimensions = TEX_MAX_DIMENSIONS;

        // Total size of header is sizeof(unsigned32)+cDDSSizeofDDSurfaceDesc2;
        const unsigned DDSSizeofDDSurfaceDesc2 = 124;
        const unsigned DDSHeaderSize = 128;

        // "DDS "
        const unsigned DDSFileSignature = 0x20534444;
        const unsigned DDS_MAGIC = DDSFileSignature;

        const unsigned DDSD_CAPS = 0x00000001;
        const unsigned DDSD_HEIGHT = 0x00000002;
        const unsigned DDSD_WIDTH = 0x00000004;
        const unsigned DDSD_PITCH = 0x00000008;

        const unsigned DDSD_BACKBUFFERCOUNT = 0x00000020;
        const unsigned DDSD_ZBUFFERBITDEPTH = 0x00000040;
        const unsigned DDSD_ALPHABITDEPTH = 0x00000080;

        const unsigned DDSD_LPSURFACE = 0x00000800;

        const unsigned DDSD_PIXELFORMAT = 0x00001000;
        const unsigned DDSD_CKDESTOVERLAY = 0x00002000;
        const unsigned DDSD_CKDESTBLT = 0x00004000;
        const unsigned DDSD_CKSRCOVERLAY = 0x00008000;

        const unsigned DDSD_CKSRCBLT = 0x00010000;
        const unsigned DDSD_MIPMAPCOUNT = 0x00020000;
        const unsigned DDSD_REFRESHRATE = 0x00040000;
        const unsigned DDSD_LINEARSIZE = 0x00080000;

        const unsigned DDSD_TEXTURESTAGE = 0x00100000;
        const unsigned DDSD_FVF = 0x00200000;
        const unsigned DDSD_SRCVBHANDLE = 0x00400000;
        const unsigned DDSD_DEPTH = 0x00800000;

        const unsigned DDSD_ALL = 0x00fff9ee;

        const unsigned DDPF_ALPHAPIXELS = 0x00000001;
        const unsigned DDPF_ALPHA = 0x00000002;
        const unsigned DDPF_FOURCC = 0x00000004;
        const unsigned DDPF_PALETTEINDEXED8 = 0x00000020;
        const unsigned DDPF_INDEXED = 0x00000020;
        const unsigned DDPF_RGB = 0x00000040;
        const unsigned DDPF_LUMINANCE = 0x00020000;

        const unsigned DDSCAPS_COMPLEX = 0x00000008;
        const unsigned DDSCAPS_TEXTURE = 0x00001000;
        const unsigned DDSCAPS_MIPMAP = 0x00400000;

        const unsigned DDSCAPS2_CUBEMAP = 0x00000200;
        const unsigned DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400;
        const unsigned DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800;

        const unsigned DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000;
        const unsigned DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000;
        const unsigned DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000;
        const unsigned DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000;

        const unsigned DDSCAPS2_VOLUME = 0x00200000;

        //  DDPIXELFORMAT
        struct PixelFormat_t
        {
            unsigned int dwSize;
            unsigned int dwFlags;
            unsigned int dwFourCC;
            unsigned int dwRGBBitCount;
            unsigned int dwRBitMask;
            unsigned int dwGBitMask;
            unsigned int dwBBitMask;
            unsigned int dwAlphaBitMask;
        };

        //  DDCAPS2
        struct Caps_t
        {
            unsigned int dwCaps1;
            unsigned int dwCaps2;
            unsigned int dwDDSX;
            unsigned int dwReserved;
        };

        X_DISABLE_WARNING(4201)
        union DDS_header
        {
            struct
            {
                unsigned int dwMagic;
                unsigned int dwSize;
                unsigned int dwFlags;
                unsigned int dwHeight;
                unsigned int dwWidth;
                unsigned int dwPitchOrLinearSize;
                unsigned int dwDepth;
                unsigned int dwMipMapCount;
                unsigned int dwReserved1[11];

                PixelFormat_t sPixelFormat;
                Caps_t sCaps;

                unsigned int dwReserved2;
            };

            X_INLINE bool isValid() const
            {
                return dwMagic == DDS_MAGIC;
            }

            char data[128];
        };
        X_ENABLE_WARNING(4201)

        struct DDS_DX10_header
        {
            DDS_DX10_header()
            {
                core::zero_this(this);
            }

            unsigned dxgiFormat;
            unsigned resourceDimension;
            unsigned miscFlag;
            unsigned arraySize;
            unsigned miscFlags2;
        };

        uint32_t ComputMaxMips(uint32_t Width, uint32_t Height)
        {
            uint32_t Biggest = core::Max<uint32_t>(Width, Height);
            uint32_t mips = 0;
            while (Biggest > 0) {
                mips++;
                Biggest >>= 1;
            }
            return mips;
        }

        namespace pixel_util
        {
            inline bool is_dxt(pixel_format fmt)
            {
                switch (fmt) {
                    case PIXEL_FMT_DXT1:
                    case PIXEL_FMT_DXT1A:
                    case PIXEL_FMT_DXT2:
                    case PIXEL_FMT_DXT3:
                    case PIXEL_FMT_DXT4:
                    case PIXEL_FMT_DXT5:
                    case PIXEL_FMT_3DC:
                    case PIXEL_FMT_DXT5A:
                    case PIXEL_FMT_DXN:
                    case PIXEL_FMT_DXT5_CCxY:
                    case PIXEL_FMT_DXT5_xGxR:
                    case PIXEL_FMT_DXT5_xGBR:
                    case PIXEL_FMT_DXT5_AGBR:
                        return true;

                        // explicit instead of fallthrough.
                        // probs allows a jmp table to be made.
                    case PIXEL_FMT_R8G8B8:
                    case PIXEL_FMT_L8:
                    case PIXEL_FMT_A8:
                    case PIXEL_FMT_A8L8:
                    case PIXEL_FMT_R8G8B8A8:
                    case PIXEL_FMT_DX10_HEADER: // humm?
                    case PIXEL_FMT_INVALID:
                        return false;

                    default:
                        break;
                }
                return false;
            }

            inline dxt_format get_dxt_format(pixel_format fmt)
            {
                switch (fmt) {
                    case PIXEL_FMT_DXT1:
                        return DXT1;
                    case PIXEL_FMT_DXT1A:
                        return DXT1A;
                    case PIXEL_FMT_DXT2:
                        return DXT3;
                    case PIXEL_FMT_DXT3:
                        return DXT3;
                    case PIXEL_FMT_DXT4:
                        return DXT5;
                    case PIXEL_FMT_DXT5:
                        return DXT5;
                    case PIXEL_FMT_3DC:
                        return DXN_YX;
                    case PIXEL_FMT_DXT5A:
                        return DXT5A;
                    case PIXEL_FMT_DXN:
                        return DXN_XY;
                    case PIXEL_FMT_DXT5_CCxY:
                        return DXT5;
                    case PIXEL_FMT_DXT5_xGxR:
                        return DXT5;
                    case PIXEL_FMT_DXT5_xGBR:
                        return DXT5;
                    case PIXEL_FMT_DXT5_AGBR:
                        return DXT5;

                    case PIXEL_FMT_INVALID:
                    case PIXEL_FMT_R8G8B8:
                    case PIXEL_FMT_L8:
                    case PIXEL_FMT_A8:
                    case PIXEL_FMT_A8L8:
                    case PIXEL_FMT_R8G8B8A8:
                    case PIXEL_FMT_DX10_HEADER:
                        return DXTInvalid;

                    default:
                        break;
                }
                return DXTInvalid;
            }

            inline pixel_format from_dxt_format(dxt_format dxt_fmt)
            {
                switch (dxt_fmt) {
                    case DXT1:
                        return PIXEL_FMT_DXT1;
                    case DXT1A:
                        return PIXEL_FMT_DXT1A;
                    case DXT3:
                        return PIXEL_FMT_DXT3;
                    case DXT5:
                        return PIXEL_FMT_DXT5;
                    case DXN_XY:
                        return PIXEL_FMT_DXN;
                    case DXN_YX:
                        return PIXEL_FMT_3DC;
                    case DXT5A:
                        return PIXEL_FMT_DXT5A;

                    case BC6_TYPELESS:
                    case BC6_UF16:
                    case BC6_SF16:
                    case BC7_TYPELESS:
                    case BC7_UNORM:
                    case BC7_UNORM_SRGB:
                    case DXTInvalid:
                        break;

                    default:
                        break;
                }
                X_ASSERT_UNREACHABLE();
                return PIXEL_FMT_INVALID;
            }

            inline unsigned get_bpp(pixel_format fmt, dxt_format dxFmt)
            {
                switch (fmt) {
                    case PIXEL_FMT_DXT1:
                        return 4;
                    case PIXEL_FMT_DXT1A:
                        return 4;
                    case PIXEL_FMT_DXT2:
                        return 8;
                    case PIXEL_FMT_DXT3:
                        return 8;
                    case PIXEL_FMT_DXT4:
                        return 8;
                    case PIXEL_FMT_DXT5:
                        return 8;
                    case PIXEL_FMT_3DC:
                        return 8;
                    case PIXEL_FMT_DXT5A:
                        return 4;
                    case PIXEL_FMT_R8G8B8:
                        return 24;
                    case PIXEL_FMT_R8G8B8A8:
                        return 32;
                    case PIXEL_FMT_A8:
                        return 8;
                    case PIXEL_FMT_L8:
                        return 8;
                    case PIXEL_FMT_A8L8:
                        return 16;
                    case PIXEL_FMT_DXN:
                        return 8;
                    case PIXEL_FMT_DXT5_CCxY:
                        return 8;
                    case PIXEL_FMT_DXT5_xGxR:
                        return 8;
                    case PIXEL_FMT_DXT5_xGBR:
                        return 8;
                    case PIXEL_FMT_DXT5_AGBR:
                        return 8;
                    case PIXEL_FMT_A16B16G16R16F:
                        return 64;
                    case PIXEL_FMT_DX10_HEADER:
                        switch (dxFmt) {
                            case dxt_format::BC6_TYPELESS:
                            case dxt_format::BC6_UF16:
                            case dxt_format::BC6_SF16:
                                X_ASSERT_NOT_IMPLEMENTED();
                                return 0;
                            case dxt_format::BC7_TYPELESS:
                            case dxt_format::BC7_UNORM:
                            case dxt_format::BC7_UNORM_SRGB:
                                return 8;
                            default:
                                break;
                        }
                    default:
                        break;
                }
                X_ASSERT_UNREACHABLE();
                return 0;
            };

            inline bool has_alpha(pixel_format fmt, dxt_format dxFmt)
            {
                switch (fmt) {
                    case PIXEL_FMT_DXT1A:
                    case PIXEL_FMT_DXT2:
                    case PIXEL_FMT_DXT3:
                    case PIXEL_FMT_DXT4:
                    case PIXEL_FMT_DXT5:
                    case PIXEL_FMT_DXT5A:
                    case PIXEL_FMT_R8G8B8A8:
                    case PIXEL_FMT_A8:
                    case PIXEL_FMT_A8L8:
                    case PIXEL_FMT_DXT5_AGBR:
                        return true;
                    case PIXEL_FMT_DX10_HEADER:
                        switch (dxFmt) {
                            case dxt_format::BC6_TYPELESS:
                            case dxt_format::BC6_UF16:
                            case dxt_format::BC6_SF16:
                                return false;
                            case dxt_format::BC7_TYPELESS:
                            case dxt_format::BC7_UNORM:
                            case dxt_format::BC7_UNORM_SRGB:
                                return true;
                            default:
                                break;
                        }
                    default:
                        break;
                }
                return false;
            }

            inline bool is_normal_map(pixel_format fmt)
            {
                switch (fmt) {
                    case PIXEL_FMT_3DC:
                    case PIXEL_FMT_DXN:
                    case PIXEL_FMT_DXT5_xGBR:
                    case PIXEL_FMT_DXT5_xGxR:
                    case PIXEL_FMT_DXT5_AGBR:
                        return true;
                    default:
                        break;
                }
                return false;
            }

            inline unsigned get_dxt_bytes_per_block(pixel_format fmt, dxt_format dxFmt)
            {
                switch (fmt) {
                    case PIXEL_FMT_DXT1:
                        return 8;
                    case PIXEL_FMT_DXT1A:
                        return 8;
                    case PIXEL_FMT_DXT5A:
                        return 8;

                    case PIXEL_FMT_DXT2:
                        return 16;
                    case PIXEL_FMT_DXT3:
                        return 16;
                    case PIXEL_FMT_DXT4:
                        return 16;
                    case PIXEL_FMT_DXT5:
                        return 16;
                    case PIXEL_FMT_3DC:
                        return 16;
                    case PIXEL_FMT_DXN:
                        return 16;
                    case PIXEL_FMT_DXT5_CCxY:
                        return 16;
                    case PIXEL_FMT_DXT5_xGxR:
                        return 16;
                    case PIXEL_FMT_DXT5_xGBR:
                        return 16;
                    case PIXEL_FMT_DXT5_AGBR:
                        return 16;

                    case PIXEL_FMT_DX10_HEADER:
                        switch (dxFmt) {
                            case dxt_format::BC6_TYPELESS:
                            case dxt_format::BC6_UF16:
                            case dxt_format::BC6_SF16:
                            case dxt_format::BC7_TYPELESS:
                            case dxt_format::BC7_UNORM:
                            case dxt_format::BC7_UNORM_SRGB:
                                return 16;

                            default:
                                break;
                        }

                    default:
                        break;
                }
                return 0;
            }

            inline Texturefmt::Enum get_text_fnt_from_pixel(pixel_format fmt, dxt_format dxFmt)
            {
                switch (fmt) {
                    case PIXEL_FMT_A8:
                        return Texturefmt::A8;
                    case PIXEL_FMT_R8G8B8:
                        return Texturefmt::R8G8B8;
                    case PIXEL_FMT_R8G8B8A8:
                        return Texturefmt::R8G8B8A8;

                    case PIXEL_FMT_DXT1:
                        return Texturefmt::BC1;
                    case PIXEL_FMT_DXT3:
                        return Texturefmt::BC2;
                    case PIXEL_FMT_DXT5:
                        return Texturefmt::BC3;

                    case PIXEL_FMT_3DC:
                        return Texturefmt::ATI2;

                    case PIXEL_FMT_A16B16G16R16F:
                        return Texturefmt::R16G16B16A16_FLOAT;

                    case PIXEL_FMT_DX10_HEADER:
                        switch (dxFmt) {
                            case dxt_format::BC6_TYPELESS:
                                return Texturefmt::BC6;
                            case dxt_format::BC6_UF16:
                                return Texturefmt::BC6;
                            case dxt_format::BC6_SF16:
                                return Texturefmt::BC6_SF16;

                            case dxt_format::BC7_TYPELESS:
                                return Texturefmt::BC7;
                            case dxt_format::BC7_UNORM:
                                return Texturefmt::BC7;
                            case dxt_format::BC7_UNORM_SRGB:
                                return Texturefmt::BC7_SRGB;
                            default:
                                break;
                        }

                    default:
                        break;
                }

                return Texturefmt::UNKNOWN;
            }

            inline uint32_t get_data_size(uint32_t width, uint32_t height,
                uint32_t mips, pixel_format fmt, dxt_format dxFmt)
            {
                uint32_t size = 0;
                uint32_t i;

                const uint32_t bits_per_pixel = get_bpp(fmt, dxFmt);
                const uint32_t bytes_per_block = get_dxt_bytes_per_block(fmt, dxFmt);
                const bool isDXT = is_dxt(fmt);

                for (i = 0; i < mips; i++) {
                    width = core::Max(1u, width);
                    height = core::Max(1u, height);

                    // work out total pixels.
                    if (isDXT)
                        size += core::Max(bytes_per_block, ((bits_per_pixel * width) * height) / 8);
                    else
                        size += core::Max(bytes_per_block, ((bits_per_pixel * width) * height) / 8);

                    // shift
                    width >>= 1;
                    height >>= 1;
                }

                return size;
            }
        } // namespace pixel_util

        X_ENSURE_SIZE(DDS_header, 128);
        X_ENSURE_SIZE(DDS_DX10_header, 20);
    } // namespace

    const char* XTexLoaderDDS::EXTENSION = DDS_FILE_EXTENSION;

    XTexLoaderDDS::XTexLoaderDDS()
    {
    }

    XTexLoaderDDS::~XTexLoaderDDS()
    {
    }

    bool XTexLoaderDDS::isValidData(const DataVec& fileData)
    {
        if (fileData.size() < sizeof(DDS_header)) {
            return false;
        }

        const DDS_header* pHdr = reinterpret_cast<const DDS_header*>(fileData.data());

        return pHdr->dwMagic == DDS_MAGIC;
    }

    // ITextureFmt
    const char* XTexLoaderDDS::getExtension(void) const
    {
        return DDS_FILE_EXTENSION;
    }

    ImgFileFormat::Enum XTexLoaderDDS::getSrcFmt(void) const
    {
        return ImgFileFormat::DDS;
    }

    bool XTexLoaderDDS::canLoadFile(const core::Path<char>& path) const
    {
        return core::strUtil::IsEqual(DDS_FILE_EXTENSION, path.extension());
    }

    bool XTexLoaderDDS::canLoadFile(const DataVec& fileData) const
    {
        return isValidData(fileData);
    }

    bool XTexLoaderDDS::loadTexture(core::XFile* file, XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_ASSERT_NOT_NULL(file);
        X_UNUSED(swapArena);

        DDS_header hdr;
        DDS_DX10_header dx10Hdr;
        uint32_t num_mip_maps, num_faces;
        dxt_format dxt_fmt = DXTInvalid;
        pixel_format format = PIXEL_FMT_INVALID;

        num_mip_maps = 1;

        if (file->readObj(hdr) != DDSHeaderSize) {
            X_ERROR("DDSLoader", "failed to read image header");
            return false;
        }

        if (!hdr.isValid()) {
            X_ERROR("DDSLoader", "image head is invalid");
            return false;
        }

        if (hdr.dwSize != DDSSizeofDDSurfaceDesc2) {
            // i've seen a dds file with valid size if ignore the 16 MSB's
            // lets support it, even if unvalid.
            uint16_t size16 = static_cast<uint16_t>(hdr.dwSize & 0xFFFF);

            if (size16 != DDSSizeofDDSurfaceDesc2) {
                X_ERROR("DDSLoader", "image header surface size is invalid. provided: %" PRIu32 " epected: 124", hdr.dwSize);
                return false;
            }
            else {
                X_WARNING("DDSLoader", "Image has malformed size field, contains data in MSB's");
            }
        }

        if (hdr.dwHeight > DDSMaxImageDimensions && hdr.dwWidth > DDSMaxImageDimensions) {
            X_ERROR("DDSLoader", "image dimensions exceed the max. provided: %" PRIu32 "x%" PRIu32 " max: %" PRIu32 "x%" PRIu32,
                hdr.dwHeight, hdr.dwWidth, DDSMaxImageDimensions, DDSMaxImageDimensions);
            return false;
        }

        if (!core::bitUtil::IsPowerOfTwo(hdr.dwHeight) || !core::bitUtil::IsPowerOfTwo(hdr.dwWidth)) {
            X_ERROR("DDSLoader", "invalid image dimensions, must be power of two. provided: %" PRIu32 "x%" PRIu32, hdr.dwHeight, hdr.dwWidth);
            return false;
        }

        if ((hdr.dwFlags & DDSD_MIPMAPCOUNT) && (hdr.dwMipMapCount)) {
            if (!(hdr.sCaps.dwCaps1 & DDSCAPS_MIPMAP)) {
                X_WARNING("DDSLoader", "DDS header is missing DDSCAPS_MIPMAP flag, yet mipcount is defined");
            }

            num_mip_maps = hdr.dwMipMapCount;

            if (num_mip_maps != ComputMaxMips(hdr.dwWidth, hdr.dwHeight)) {
                X_ERROR("DDSLoader", "mip map count is incorrect. provided: %" PRIu32 " expected: %" PRIu32, num_mip_maps,
                    ComputMaxMips(hdr.dwWidth, hdr.dwHeight));
                return false;
            }
        }

        // Check for faces / volume
        if (hdr.sCaps.dwCaps1 & DDSCAPS_COMPLEX) {
            if (hdr.sCaps.dwCaps2 & DDSCAPS2_CUBEMAP) {
                const uint32_t all_faces_mask = DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ;
                if ((hdr.sCaps.dwCaps2 & all_faces_mask) != all_faces_mask) {
                    X_ERROR("DDSLoader", "cubemaps must have 6 faces");
                    return false;
                }

                num_faces = 6; // cubemap :)
            }
            else if (hdr.sCaps.dwCaps2 & DDSCAPS2_VOLUME) {
                X_ERROR("DDSLoader", "Volume textures unsupported");
                return false;
            }
            else {
                num_faces = 1;
            }
        }
        else {
            num_faces = 1;
        }

        // what are you doing!
        if (hdr.sPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            X_ERROR("DDSLoader", "Palettized textures unsupported");
            return false;
        }

        if (hdr.sPixelFormat.dwFlags & DDPF_FOURCC) {
            // http://code.google.com/p/nvidia-texture-tools/issues/detail?id=41
            // ATI2 YX:            0 (0x00000000)
            // ATI2 XY:   1498952257 (0x59583241) (BC5)
            // ATI Compressonator obeys this stuff, nvidia's tools (like readdxt) don't - oh great

            switch (hdr.sPixelFormat.dwFourCC) {
                case PIXEL_FMT_DXT1: {
                    format = PIXEL_FMT_DXT1;
                    dxt_fmt = DXT1;
                    break;
                }
                case PIXEL_FMT_DXT2:
                case PIXEL_FMT_DXT3: {
                    format = PIXEL_FMT_DXT3;
                    dxt_fmt = DXT3;
                    break;
                }
                case PIXEL_FMT_DXT4:
                case PIXEL_FMT_DXT5: {
                    switch (hdr.sPixelFormat.dwRGBBitCount) {
                        case PIXEL_FMT_DXT5_CCxY:
                            format = PIXEL_FMT_DXT5_CCxY;
                            break;
                        case PIXEL_FMT_DXT5_xGxR:
                            format = PIXEL_FMT_DXT5_xGxR;
                            break;
                        case PIXEL_FMT_DXT5_xGBR:
                            format = PIXEL_FMT_DXT5_xGBR;
                            break;
                        case PIXEL_FMT_DXT5_AGBR:
                            format = PIXEL_FMT_DXT5_AGBR;
                            break;
                        default:
                            format = PIXEL_FMT_DXT5;
                            break;
                    }

                    dxt_fmt = DXT5;
                    break;
                }
                case PIXEL_FMT_3DC: {
                    if (hdr.sPixelFormat.dwRGBBitCount == PIXEL_FMT_DXN) {
                        dxt_fmt = DXN_XY;
                        format = PIXEL_FMT_DXN;
                    }
                    else {
                        dxt_fmt = DXN_YX; // aka ATI2
                        format = PIXEL_FMT_3DC;
                    }
                    break;
                }
                case PIXEL_FMT_DXT5A: {
                    format = PIXEL_FMT_DXT5A;
                    dxt_fmt = DXT5A;
                    break;
                }

                case PIXEL_FMT_DX10_HEADER:
                    if (file->readObj(dx10Hdr) != sizeof(dx10Hdr)) {
                        X_ERROR("DDSLoader", "Failed to read DX10 Header");
                        return false;
                    }

                    format = PIXEL_FMT_DX10_HEADER;

                    switch (dx10Hdr.dxgiFormat) {
                        case dxgiFormat::BC6H_TYPELESS:
                            dxt_fmt = dxt_format::BC6_TYPELESS;
                            break;
                        case dxgiFormat::BC6H_UF16:
                            dxt_fmt = dxt_format::BC7_UNORM;
                            break;
                        case dxgiFormat::BC6H_SF16:
                            dxt_fmt = dxt_format::BC7_UNORM_SRGB;
                            break;

                        case dxgiFormat::BC7_TYPELESS:
                            dxt_fmt = dxt_format::BC7_TYPELESS;
                            break;
                        case dxgiFormat::BC7_UNORM:
                            dxt_fmt = dxt_format::BC7_UNORM;
                            break;
                        case dxgiFormat::BC7_UNORM_SRGB:
                            dxt_fmt = dxt_format::BC7_UNORM_SRGB;
                            break;

                        default:
                            X_ERROR("DDSLoader", "Unsupported DX10 format: 0x%08X", dx10Hdr.dxgiFormat);
                            return false;
                            break;
                    }

                    break;

                case PIXEL_FMT_A16B16G16R16F:
                    format = PIXEL_FMT_A16B16G16R16F;
                    break;

                default: {
                    X_ERROR("DDSLoader", "Unsupported DDS FOURCC format: 0x%08X", hdr.sPixelFormat.dwFourCC);
                    return false;
                }
            }
        }
        else if ((hdr.sPixelFormat.dwRGBBitCount < 8) || (hdr.sPixelFormat.dwRGBBitCount > 32) || (hdr.sPixelFormat.dwRGBBitCount & 7)) {
            X_ERROR("DDSLoader", "Unsupported bit count: %" PRIu32, hdr.sPixelFormat.dwRGBBitCount);
            return false;
        }
        else if (hdr.sPixelFormat.dwFlags & DDPF_RGB) {
            if (hdr.sPixelFormat.dwFlags & DDPF_LUMINANCE) {
                if (hdr.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {
                    format = PIXEL_FMT_A8L8;
                }
                else {
                    format = PIXEL_FMT_L8;
                }
            }
            else if (hdr.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {
                format = PIXEL_FMT_R8G8B8A8;
            }
            else {
                format = PIXEL_FMT_R8G8B8;
            }
        }
        else if (hdr.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {
            if (hdr.sPixelFormat.dwFlags & DDPF_LUMINANCE) {
                format = PIXEL_FMT_A8L8;
            }
            else {
                format = PIXEL_FMT_A8;
            }
        }
        else if (hdr.sPixelFormat.dwFlags & DDPF_LUMINANCE) {
            format = PIXEL_FMT_L8;
        }
        else if (hdr.sPixelFormat.dwFlags & DDPF_ALPHA) {
            format = PIXEL_FMT_A8;
        }
        else {
            X_ERROR("DDSLoader", "Unsupported format");
            return false;
        }

        uint32_t bits_per_pixel = hdr.sPixelFormat.dwRGBBitCount;
        uint32_t pitch = hdr.dwPitchOrLinearSize;
        uint32_t default_pitch;
        uint32_t mask_size[4];
        uint32_t mask_ofs[4];

        if (hdr.sPixelFormat.dwFlags & DDPF_FOURCC) {
            bits_per_pixel = pixel_util::get_bpp(format, dxt_fmt);
        }

        if (hdr.sPixelFormat.dwFlags & DDPF_FOURCC) {
            default_pitch = (((hdr.dwWidth + 3) & ~3) * ((hdr.dwHeight + 3) & ~3) * bits_per_pixel) >> 3;
        }
        else {
            default_pitch = (hdr.dwWidth * bits_per_pixel) >> 3;
        }

        if (!pitch) {
            pitch = default_pitch;
        }
        else if (pitch > default_pitch * 8) {
            X_ERROR("DDSLoader", "Pitch Error");
            return false;
        }

        // map the format
        Texturefmt::Enum mapped_format = Texturefmt::UNKNOWN;
        mapped_format = pixel_util::get_text_fnt_from_pixel(format, dxt_fmt);

        if (mapped_format == Texturefmt::UNKNOWN) {
            X_ERROR("DDSLoader", "Unsupported supported.");
            return false;
        }

        if (core::bitUtil::IsBitFlagSet(hdr.sPixelFormat.dwFlags, DDPF_RGB)) {
            mask_size[0] = core::bitUtil::CountBits(hdr.sPixelFormat.dwRBitMask);
            mask_size[1] = core::bitUtil::CountBits(hdr.sPixelFormat.dwGBitMask);
            mask_size[2] = core::bitUtil::CountBits(hdr.sPixelFormat.dwBBitMask);
            mask_size[3] = core::bitUtil::CountBits(hdr.sPixelFormat.dwAlphaBitMask);

            mask_ofs[0] = core::bitUtil::ScanBitsForward(hdr.sPixelFormat.dwRBitMask);
            mask_ofs[1] = core::bitUtil::ScanBitsForward(hdr.sPixelFormat.dwGBitMask);
            mask_ofs[2] = core::bitUtil::ScanBitsForward(hdr.sPixelFormat.dwBBitMask);
            mask_ofs[3] = core::bitUtil::ScanBitsForward(hdr.sPixelFormat.dwAlphaBitMask);

            if ((hdr.sPixelFormat.dwFlags & DDPF_LUMINANCE) && (!mask_size[0])) {
                mask_size[0] = hdr.sPixelFormat.dwRGBBitCount >> 3;
                if (hdr.sPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {
                    mask_size[0] /= 2;
                }
            }

            // check mask sizes.
            if (mapped_format == Texturefmt::R8G8B8A8 || mapped_format == Texturefmt::R8G8B8) {
                if (mask_size[0] != 8 || mask_size[1] != 8 || mask_size[2] != 8 || mask_size[3] != 8) {
                    bool ValidRGB = (mapped_format == Texturefmt::R8G8B8 && mask_size[3] == 0);
                    if (!ValidRGB) {
                        X_ERROR("DDSLoader", "Invalid mask sizes expected 8. (%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ")",
                            mask_size[0], mask_size[1], mask_size[2], mask_size[3]);
                        return false;
                    }
                }

                // check for swizle
                if (mapped_format == Texturefmt::R8G8B8A8) {
                    if (mask_ofs[0] == 16 && mask_ofs[1] == 8 && mask_ofs[2] == 0
                        && mask_ofs[3] == 24) {
                        // this is BGRA
                        mapped_format = Texturefmt::B8G8R8A8;
                    }
                    else if (mask_ofs[0] != 0 || mask_ofs[1] != 8 || mask_ofs[2] != 16
                             || mask_ofs[3] != 24) {
                        // this is not a valid RGBA
                        X_ERROR("DDSLoader", "Invalid pixel offsets for R8G8B8A8 expected(0,8,16,24) provided(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ")",
                            mask_ofs[0], mask_ofs[1], mask_ofs[2], mask_ofs[3]);
                        return false;
                    }
                }
                else if (mapped_format == Texturefmt::R8G8B8) {
                    if (mask_ofs[0] == 16 && mask_ofs[1] == 8 && mask_ofs[2] == 0) {
                        // this is BGR
                        mapped_format = Texturefmt::B8G8R8;
                    }
                    else if (mask_ofs[0] != 0 || mask_ofs[1] != 8 || mask_ofs[2] != 16
                             || mask_ofs[3] != core::bitUtil::NO_BIT_SET) {
                        // make it zero for log
                        if (mask_ofs[3] == core::bitUtil::NO_BIT_SET) {
                            mask_ofs[3] = 0;
                        }

                        // this is not a valid RGB
                        X_ERROR("DDSLoader", "Invalid pixel offsets for R8G8B8 expected(0,8,16,0) provided(%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%" PRIu32 ")",
                            mask_ofs[0], mask_ofs[1], mask_ofs[2], mask_ofs[3]);
                        return false;
                    }
                }
            }
        }

        // load the image data.
        TextureFlags flags;

        if (pixel_util::has_alpha(format, dxt_fmt)) {
            flags.Set(TextureFlags::ALPHA);
        }
        if (pixel_util::is_normal_map(format)) {
            flags.Set(TextureFlags::NORMAL);
        }
        if (num_mip_maps == 1) {
            flags.Set(TextureFlags::NOMIPS);
        }
        if (num_faces == 1) {
            imgFile.setType(TextureType::T2D);
        }
        else {
            imgFile.setType(TextureType::TCube);
        }

        // set the info
        imgFile.setWidth(safe_static_cast<uint16_t, uint32_t>(hdr.dwWidth));
        imgFile.setHeigth(safe_static_cast<uint16_t, uint32_t>(hdr.dwHeight));
        imgFile.setNumMips(safe_static_cast<int32_t, uint32_t>(num_mip_maps));
        imgFile.setNumFaces(safe_static_cast<int32_t, uint32_t>(num_faces)); // 1 for 2D 6 for a cube.
        imgFile.setDepth(1);                                                 /// We Don't allow volume texture loading yet.
        imgFile.setFlags(flags);
        imgFile.setFormat(mapped_format);
        imgFile.resize();

        uint32_t i, bytes_read;
        uint32_t total_bytes_per_face = pixel_util::get_data_size(hdr.dwWidth, hdr.dwHeight, num_mip_maps, format, dxt_fmt);

        X_ASSERT(imgFile.getFaceSize() == total_bytes_per_face, "Face size mismatch")(imgFile.getFaceSize(), total_bytes_per_face); 

        // allocate memory / read.
        // no idear if allocating then reading has any benfits for cube maps.
        for (i = 0; i < num_faces; i++) {
            bytes_read = safe_static_cast<uint32_t, size_t>(file->read(imgFile.getFace(i), total_bytes_per_face));

            if (bytes_read != total_bytes_per_face) {
                X_ERROR("DDSLoader", "failed to read all mips. requested: %" PRIu32 " bytes got: %" PRIu32 " bytes",
                    total_bytes_per_face, bytes_read);
                return false;
            }
        }

#if X_DEBUG == 1
        const uint64_t left = file->remainingBytes();
        X_WARNING_IF(left > 0, "DDSLoader", "potential read fail, bytes left in file: %" PRIu64, left);
#endif

        return true;
    }

    bool XTexLoaderDDS::saveTexture(core::XFile* file, const XTextureFile& imgFile, core::MemoryArenaBase* swapArena)
    {
        X_UNUSED(file, imgFile, swapArena);

        if (imgFile.getDepth() > 1) {
            X_ERROR("DDS", "Depth not supported.");
            return false;
        }

        DDS_header hdr;
        DDS_DX10_header dx10Hdr;

        core::zero_object(hdr);
        core::zero_object(dx10Hdr);

        hdr.dwMagic = DDS_MAGIC;
        hdr.dwSize = DDSSizeofDDSurfaceDesc2;
        hdr.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
        hdr.dwHeight = imgFile.getHeight();
        hdr.dwWidth = imgFile.getWidth();
        hdr.dwPitchOrLinearSize = 0;
        hdr.dwDepth = 0;
        hdr.dwMipMapCount = 0;

        hdr.sPixelFormat.dwSize = sizeof(hdr.sPixelFormat);
        hdr.sPixelFormat.dwFlags = 0;
        hdr.sPixelFormat.dwFourCC = 0;
        hdr.sPixelFormat.dwRGBBitCount = 0;
        hdr.sPixelFormat.dwRBitMask = 0;
        hdr.sPixelFormat.dwGBitMask = 0;
        hdr.sPixelFormat.dwBBitMask = 0;
        hdr.sPixelFormat.dwAlphaBitMask = 0;

        hdr.sCaps.dwCaps1 = 0;
        hdr.sCaps.dwCaps2 = 0;
        hdr.sCaps.dwDDSX = 0;
        hdr.sCaps.dwReserved = 0;

        if (imgFile.getNumMips() > 1) {
            hdr.dwMipMapCount = imgFile.getNumMips();
            hdr.dwFlags |= DDSD_MIPMAPCOUNT;
            hdr.sCaps.dwCaps1 |= (DDSCAPS_MIPMAP | DDSCAPS_COMPLEX);
        }

        if (Util::isDxt(imgFile.getFormat())) {
            auto bitsPerPixel = Util::bitsPerPixel(imgFile.getFormat());

            hdr.sPixelFormat.dwFlags |= DDPF_FOURCC;
            hdr.dwPitchOrLinearSize = (((hdr.dwWidth + 3) & ~3) * ((hdr.dwHeight + 3) & ~3) * bitsPerPixel) >> 3;
            hdr.dwFlags |= DDSD_LINEARSIZE;

            switch (imgFile.getFormat()) {
                case Texturefmt::BC1:
                    hdr.sPixelFormat.dwFlags |= DDPF_FOURCC;
                    hdr.sPixelFormat.dwFourCC = PIXEL_FMT_DXT1;
                    break;
                case Texturefmt::BC2:
                    hdr.sPixelFormat.dwFlags |= DDPF_FOURCC;
                    hdr.sPixelFormat.dwFourCC = PIXEL_FMT_DXT3;
                    break;
                case Texturefmt::BC3:
                    hdr.sPixelFormat.dwFlags |= DDPF_FOURCC;
                    hdr.sPixelFormat.dwFourCC = PIXEL_FMT_DXT5;
                    break;

                // ATI1
                case Texturefmt::BC4:
                    hdr.sPixelFormat.dwFlags |= DDPF_FOURCC;
                    hdr.sPixelFormat.dwFourCC = PIXEL_FMT_DXT1;
                    break;

                // ATI2
                case Texturefmt::BC5:
                    hdr.sPixelFormat.dwFlags |= DDPF_FOURCC;
                    hdr.sPixelFormat.dwFourCC = PIXEL_FMT_DXT1;
                    break;

                case Texturefmt::BC6:
                case Texturefmt::BC6_SF16:
                case Texturefmt::BC6_TYPELESS:
                case Texturefmt::BC7:
                case Texturefmt::BC7_SRGB:
                case Texturefmt::BC7_TYPELESS:
                    hdr.sPixelFormat.dwFourCC = PIXEL_FMT_DX10_HEADER;
                    hdr.sPixelFormat.dwRGBBitCount = 0;

                    switch (imgFile.getFormat()) {
                        case Texturefmt::BC6:
                            dx10Hdr.dxgiFormat = dxgiFormat::BC6H_UF16;
                            break;
                        case Texturefmt::BC6_SF16:
                            dx10Hdr.dxgiFormat = dxgiFormat::BC6H_SF16;
                            break;
                        case Texturefmt::BC6_TYPELESS:
                            dx10Hdr.dxgiFormat = dxgiFormat::BC6H_TYPELESS;
                            break;
                        case Texturefmt::BC7:
                            dx10Hdr.dxgiFormat = dxgiFormat::BC7_UNORM;
                            break;
                        case Texturefmt::BC7_TYPELESS:
                            dx10Hdr.dxgiFormat = dxgiFormat::BC7_TYPELESS;
                            break;
                        case Texturefmt::BC7_SRGB:
                            dx10Hdr.dxgiFormat = dxgiFormat::BC7_UNORM_SRGB;
                            break;
                        default:
                            X_ASSERT_UNREACHABLE();
                            break;
                    }

                    break;

                default:
                    X_ERROR("DDS", "Unsupported dxt fmt: %s", Texturefmt::ToString(imgFile.getFormat()));
                    return false;
            }
        }
        else 
        {
            switch (imgFile.getFormat()) 
            {
                case Texturefmt::A8:
                    hdr.sPixelFormat.dwFlags |= DDPF_ALPHA;
                    hdr.sPixelFormat.dwFourCC = 0;
                    hdr.sPixelFormat.dwRGBBitCount = 8;
                    hdr.sPixelFormat.dwRBitMask = 0x00000000;
                    hdr.sPixelFormat.dwGBitMask = 0x00000000;
                    hdr.sPixelFormat.dwBBitMask = 0x00000000;
                    hdr.sPixelFormat.dwAlphaBitMask = 0x000000ff;
                    break;
                case Texturefmt::R8G8B8A8:
                    hdr.sPixelFormat.dwFlags |= DDPF_RGB | DDPF_ALPHAPIXELS;
                    hdr.sPixelFormat.dwFourCC = PIXEL_FMT_R8G8B8A8;
                    hdr.sPixelFormat.dwRGBBitCount = 32;
                    hdr.sPixelFormat.dwRBitMask = 0x000000ff;
                    hdr.sPixelFormat.dwGBitMask = 0x0000ff00;
                    hdr.sPixelFormat.dwBBitMask = 0x00ff0000;
                    hdr.sPixelFormat.dwAlphaBitMask = 0xff000000;
                    break;
                default:
                    X_ERROR("DDS", "Unsupported fmt: %s", Texturefmt::ToString(imgFile.getFormat()));
                    return false;
            }
        }

        if (file->writeObj(hdr) != sizeof(hdr)) {
            X_ERROR("DDS", "Failed to write header");
            return false;
        }

        if (hdr.sPixelFormat.dwFourCC == PIXEL_FMT_DX10_HEADER) {
            if (file->writeObj(dx10Hdr) != sizeof(dx10Hdr)) {
                X_ERROR("DDS", "Failed to write dx10 header");
                return false;
            }
        }

        for (size_t i = 0; i < imgFile.getNumFaces(); i++) {
            size_t faceSize = imgFile.getFaceSize();

            if (file->write(imgFile.getFace(i), faceSize) != faceSize) {
                X_ERROR("DDS", "Failed to write face");
                return false;
            }
        }

        return true;
    }

    // ~ITextureFmt

} // namespace DDS

X_NAMESPACE_END