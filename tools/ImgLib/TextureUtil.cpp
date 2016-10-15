#include "stdafx.h"
#include "TextureUtil.h"

#include <Hashing\Fnva1Hash.h>

X_NAMESPACE_BEGIN(texture)

namespace Util
{

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
		switch (fmt)
		{
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
		switch (fmt)
		{
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
		default: break;
		}
		return false;
	}

	uint32_t bitsPerPixel(Texturefmt::Enum fmt)
	{
		switch (fmt)
		{
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
		switch (fmt)
		{
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
		if (isDXT)
		{
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
		unsigned size = 0;
		unsigned i;

		const unsigned bits_per_pixel = bitsPerPixel(fmt);
		const bool isDXT = isDxt(fmt);

		unsigned bytes_per_block = 0;

		if (isDXT) {
			bytes_per_block = dxtBytesPerBlock(fmt);
		}

		for (i = 0; i < mips; i++)
		{
			width = core::Max(1u, width);
			height = core::Max(1u, height);

			// work out total pixels.
			if (isDXT)
			{
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

	size_t rowBytes(uint32_t width, uint32_t height, Texturefmt::Enum fmt)
	{
		size_t rowBytes = 0;

		bool blockComp = false;
		bool packed = false;
		bool planar = false;
		size_t bytesPerElem = 0;

		switch (fmt)
		{
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

		if (blockComp)
		{
			size_t numBlocksWide = 0;
			if (width > 0)
			{
				numBlocksWide = core::Max<size_t>(1, (width + 3) / 4);
			}
			size_t numBlocksHigh = 0;
			if (height > 0)
			{
				numBlocksHigh = core::Max<size_t>(1, (height + 3) / 4);
			}
			rowBytes = numBlocksWide * bytesPerElem;
		}
		else if (packed)
		{
			rowBytes = ((width + 1) >> 1) * bytesPerElem;
		}
		else if (planar)
		{
			rowBytes = ((width + 1) >> 1) * bytesPerElem;
		}
		else
		{
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


		switch (fmt)
		{
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

		if (blockComp)
		{
			size_t numBlocksWide = 0;
			if (width > 0)
			{
				numBlocksWide = core::Max<size_t>(1, (width + 3) / 4);
			}
			size_t numBlocksHigh = 0;
			if (height > 0)
			{
				numBlocksHigh = core::Max<size_t>(1, (height + 3) / 4);
			}
			rowBytes = numBlocksWide * bytesPerElem;
			numRows = numBlocksHigh;
			numBytes = rowBytes * numBlocksHigh;
		}
		else if (packed)
		{
			rowBytes = ((width + 1) >> 1) * bytesPerElem;
			numRows = height;
			numBytes = rowBytes * height;
		}
		else if (planar)
		{
			rowBytes = ((width + 1) >> 1) * bytesPerElem;
			numBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
			numRows = height + ((height + 1) >> 1);
		}
		else
		{
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

		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
		{
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

			case "R16G16B16A16_FLOAT"_fnv1a:
				return Texturefmt::R16G16B16A16_FLOAT;

			default:
				// this is here incase i've forgot to add a new format
				// into this function.
				X_ASSERT_UNREACHABLE();
				return Texturefmt::UNKNOWN;
		}
	}


} // namespace Util

X_NAMESPACE_END
