#include "stdafx.h"
#include "TextureUtil.h"

X_NAMESPACE_BEGIN(texture)

namespace Util
{
    DXGI_FORMAT DXGIFormatFromTexFmt(Texturefmt::Enum fmt)
    {
        static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

        switch (fmt) {
            case Texturefmt::R8G8B8:
                return DXGI_FORMAT_R8G8B8A8_UNORM; // is this right?
            case Texturefmt::R8G8B8A8:
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            case Texturefmt::B8G8R8A8:
                return DXGI_FORMAT_B8G8R8A8_UNORM;

            case Texturefmt::A8:
                return DXGI_FORMAT_A8_UNORM;

            case Texturefmt::A8R8G8B8:
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            // 16bit
            case Texturefmt::R16G16_FLOAT:
                return DXGI_FORMAT_R16G16_FLOAT;
            case Texturefmt::R16G16:
                return DXGI_FORMAT_R16G16_UNORM;
            case Texturefmt::R16G16_SNORM:
                return DXGI_FORMAT_R16G16_SNORM;
            case Texturefmt::R16G16_SINT:
                return DXGI_FORMAT_R16G16_SINT;
            case Texturefmt::R16G16_UINT:
                return DXGI_FORMAT_R16G16_UINT;
            case Texturefmt::R16G16_TYPELESS:
                return DXGI_FORMAT_R16G16_TYPELESS;

            case Texturefmt::R16G16B16A16_FLOAT:
                return DXGI_FORMAT_R16G16B16A16_FLOAT;

            // bc*
            case Texturefmt::BC1:
                return DXGI_FORMAT_BC1_UNORM;
            case Texturefmt::BC2:
                return DXGI_FORMAT_BC2_UNORM;
            case Texturefmt::BC3:
                return DXGI_FORMAT_BC3_UNORM;
            case Texturefmt::BC4:
                return DXGI_FORMAT_BC4_UNORM;
            case Texturefmt::BC4_SNORM:
                return DXGI_FORMAT_BC4_SNORM;

            case Texturefmt::BC5:
            case Texturefmt::ATI2:
                return DXGI_FORMAT_BC5_UNORM;
            case Texturefmt::BC5_SNORM:
                return DXGI_FORMAT_BC5_SNORM;

            case Texturefmt::BC6:
                return DXGI_FORMAT_BC6H_UF16; // HDR BAbbbbbbbbby!
            case Texturefmt::BC6_SF16:
                return DXGI_FORMAT_BC6H_SF16;
            case Texturefmt::BC6_TYPELESS:
                return DXGI_FORMAT_BC6H_TYPELESS;

            case Texturefmt::BC7:
                return DXGI_FORMAT_BC7_UNORM;
            case Texturefmt::BC7_SRGB:
                return DXGI_FORMAT_BC7_UNORM_SRGB;
            case Texturefmt::BC7_TYPELESS:
                return DXGI_FORMAT_BC7_TYPELESS;

            case Texturefmt::R10G10B10A2:
                return DXGI_FORMAT_R10G10B10A2_UNORM;
            case Texturefmt::R10G10B10A2_UINT:
                return DXGI_FORMAT_R10G10B10A2_UINT;
            case Texturefmt::R10G10B10A2_TYPELESS:
                return DXGI_FORMAT_R10G10B10A2_TYPELESS;

            case Texturefmt::R24G8_TYPELESS:
                return DXGI_FORMAT_R24G8_TYPELESS;

            case Texturefmt::D24_UNORM_S8_UNIT:
                return DXGI_FORMAT_D24_UNORM_S8_UINT;

            case Texturefmt::D32_FLOAT:
                return DXGI_FORMAT_D32_FLOAT;

#if X_DEBUG
            default:
                X_ASSERT_UNREACHABLE();
                return DXGI_FORMAT_UNKNOWN;
#else
            default:
                X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
        }
    }

    Texturefmt::Enum texFmtFromDXGI(DXGI_FORMAT fmt)
    {
        switch (fmt) {
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return Texturefmt::R8G8B8A8;
            case DXGI_FORMAT_B8G8R8A8_UNORM:
                return Texturefmt::B8G8R8A8;
            case DXGI_FORMAT_A8_UNORM:
                return Texturefmt::A8;

            // 16bit
            case DXGI_FORMAT_R16G16_UNORM:
                return Texturefmt::R16G16;
            case DXGI_FORMAT_R16G16_FLOAT:
                return Texturefmt::R16G16_FLOAT;
            case DXGI_FORMAT_R16G16_SNORM:
                return Texturefmt::R16G16_SNORM;
            case DXGI_FORMAT_R16G16_SINT:
                return Texturefmt::R16G16_SINT;
            case DXGI_FORMAT_R16G16_UINT:
                return Texturefmt::R16G16_UINT;
            case DXGI_FORMAT_R16G16_TYPELESS:
                return Texturefmt::R16G16_TYPELESS;

            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                return Texturefmt::R16G16B16A16_FLOAT;

            case DXGI_FORMAT_BC1_UNORM:
                return Texturefmt::BC1;
            case DXGI_FORMAT_BC2_UNORM:
                return Texturefmt::BC2;
            case DXGI_FORMAT_BC3_UNORM:
                return Texturefmt::BC3;
            case DXGI_FORMAT_BC4_UNORM:
                return Texturefmt::BC4;
            case DXGI_FORMAT_BC5_UNORM:
                return Texturefmt::BC5;

            case DXGI_FORMAT_BC1_UNORM_SRGB:
                return Texturefmt::BC1_SRGB;
            case DXGI_FORMAT_BC2_UNORM_SRGB:
                return Texturefmt::BC2_SRGB;
            case DXGI_FORMAT_BC3_UNORM_SRGB:
                return Texturefmt::BC3_SRGB;

            case DXGI_FORMAT_BC4_SNORM:
                return Texturefmt::BC4_SNORM;
            case DXGI_FORMAT_BC5_SNORM:
                return Texturefmt::BC5_SNORM;

            case DXGI_FORMAT_BC1_TYPELESS:
                return Texturefmt::BC1_TYPELESS;
            case DXGI_FORMAT_BC2_TYPELESS:
                return Texturefmt::BC2_TYPELESS;
            case DXGI_FORMAT_BC3_TYPELESS:
                return Texturefmt::BC3_TYPELESS;
            case DXGI_FORMAT_BC4_TYPELESS:
                return Texturefmt::BC4_TYPELESS;
            case DXGI_FORMAT_BC5_TYPELESS:
                return Texturefmt::BC5_TYPELESS;

            case DXGI_FORMAT_BC6H_UF16:
                return Texturefmt::BC6;
            case DXGI_FORMAT_BC6H_SF16:
                return Texturefmt::BC6_SF16;
            case DXGI_FORMAT_BC6H_TYPELESS:
                return Texturefmt::BC6_TYPELESS;

            case DXGI_FORMAT_BC7_UNORM:
                return Texturefmt::BC7;
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return Texturefmt::BC7_SRGB;
            case DXGI_FORMAT_BC7_TYPELESS:
                return Texturefmt::BC7_TYPELESS;

            case DXGI_FORMAT_R10G10B10A2_UNORM:
                return Texturefmt::R10G10B10A2;
            case DXGI_FORMAT_R10G10B10A2_UINT:
                return Texturefmt::R10G10B10A2_UINT;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
                return Texturefmt::R10G10B10A2_TYPELESS;

            case DXGI_FORMAT_R24G8_TYPELESS:
                return Texturefmt::R24G8_TYPELESS;
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
                return Texturefmt::D24_UNORM_S8_UNIT;
            case DXGI_FORMAT_D32_FLOAT:
                return Texturefmt::D32_FLOAT;

#if X_DEBUG
            default:
                X_ASSERT_UNREACHABLE();
                return Texturefmt::UNKNOWN;
#else
            default:
                X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
        }
    }

} // namespace Util

X_NAMESPACE_END