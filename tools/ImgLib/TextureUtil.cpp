#include "stdafx.h"
#include "TextureUtil.h"


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

		if (core::strUtil::IsEqualCaseInsen("A8", pStr))
			return Texturefmt::A8;

		if (core::strUtil::IsEqualCaseInsen("R8G8", pStr))
			return Texturefmt::R8G8;
		if (core::strUtil::IsEqualCaseInsen("R8G8_TYPELESS", pStr))
			return Texturefmt::R8G8_TYPELESS;
		if (core::strUtil::IsEqualCaseInsen("R8G8_SNORM", pStr))
			return Texturefmt::R8G8_SNORM;
		if (core::strUtil::IsEqualCaseInsen("R8G8_UNIT", pStr))
			return Texturefmt::R8G8_UNIT;
		if (core::strUtil::IsEqualCaseInsen("R8G8_SINT", pStr))
			return Texturefmt::R8G8_SINT;

		if (core::strUtil::IsEqualCaseInsen("R16G16_FLOAT", pStr))
			return Texturefmt::R16G16_FLOAT;
		if (core::strUtil::IsEqualCaseInsen("R16G16", pStr))
			return Texturefmt::R16G16;
		if (core::strUtil::IsEqualCaseInsen("R16G16_SRGB", pStr))
			return Texturefmt::R16G16_SRGB;
		if (core::strUtil::IsEqualCaseInsen("R16G16_SNORM", pStr))
			return Texturefmt::R16G16_SNORM;
		if (core::strUtil::IsEqualCaseInsen("R16G16_SINT", pStr))
			return Texturefmt::R16G16_SINT;
		if (core::strUtil::IsEqualCaseInsen("R16G16_UINT", pStr))
			return Texturefmt::R16G16_UINT;
		if (core::strUtil::IsEqualCaseInsen("R16G16_TYPELESS", pStr))
			return Texturefmt::R16G16_TYPELESS;


		if (core::strUtil::IsEqualCaseInsen("R8G8B8", pStr))
			return Texturefmt::R8G8B8;
		if (core::strUtil::IsEqualCaseInsen("B8G8R8", pStr))
			return Texturefmt::B8G8R8;


		if (core::strUtil::IsEqualCaseInsen("R8G8B8A8", pStr))
			return Texturefmt::R8G8B8A8;
		if (core::strUtil::IsEqualCaseInsen("R8G8B8A8_SRGB", pStr))
			return Texturefmt::R8G8B8A8_SRGB;
		if (core::strUtil::IsEqualCaseInsen("R8G8B8A8_SNORM", pStr))
			return Texturefmt::R8G8B8A8_SNORM;
		if (core::strUtil::IsEqualCaseInsen("R8G8B8A8_TYPELESS", pStr))
			return Texturefmt::R8G8B8A8_TYPELESS;
		if (core::strUtil::IsEqualCaseInsen("R8G8B8A8_SINT", pStr))
			return Texturefmt::R8G8B8A8_SINT;
		if (core::strUtil::IsEqualCaseInsen("R8G8B8A8_UINT", pStr))
			return Texturefmt::R8G8B8A8_UINT;


		if (core::strUtil::IsEqualCaseInsen("B8G8R8A8", pStr))
			return Texturefmt::B8G8R8A8;
		if (core::strUtil::IsEqualCaseInsen("B8G8R8A8_SRGB", pStr))
			return Texturefmt::B8G8R8A8_SRGB;
		if (core::strUtil::IsEqualCaseInsen("B8G8R8A8_TYPELESS", pStr))
			return Texturefmt::B8G8R8A8_TYPELESS;


		if (core::strUtil::IsEqualCaseInsen("ATI2", pStr))
			return Texturefmt::ATI2;
		if (core::strUtil::IsEqualCaseInsen("ATI2_XY", pStr))
			return Texturefmt::ATI2_XY;


		if (core::strUtil::IsEqualCaseInsen("BC1", pStr))
			return Texturefmt::BC1;
		if (core::strUtil::IsEqualCaseInsen("BC1_SRGB", pStr))
			return Texturefmt::BC1_SRGB;
		if (core::strUtil::IsEqualCaseInsen("BC1_TYPELESS", pStr))
			return Texturefmt::BC1_TYPELESS;


		if (core::strUtil::IsEqualCaseInsen("BC2", pStr))
			return Texturefmt::BC2;
		if (core::strUtil::IsEqualCaseInsen("BC2_SRGB", pStr))
			return Texturefmt::BC2_SRGB;
		if (core::strUtil::IsEqualCaseInsen("BC2_TYPELESS", pStr))
			return Texturefmt::BC2_TYPELESS;


		if (core::strUtil::IsEqualCaseInsen("BC3", pStr))
			return Texturefmt::BC3;
		if (core::strUtil::IsEqualCaseInsen("BC3_SRGB", pStr))
			return Texturefmt::BC3_SRGB;
		if (core::strUtil::IsEqualCaseInsen("BC3_TYPELESS", pStr))
			return Texturefmt::BC3_TYPELESS;


		if (core::strUtil::IsEqualCaseInsen("BC4", pStr))
			return Texturefmt::BC4;
		if (core::strUtil::IsEqualCaseInsen("BC4_SNORM", pStr))
			return Texturefmt::BC4_SNORM;
		if (core::strUtil::IsEqualCaseInsen("BC4_TYPELESS", pStr))
			return Texturefmt::BC4_TYPELESS;

		if (core::strUtil::IsEqualCaseInsen("BC5", pStr))
			return Texturefmt::BC5;
		if (core::strUtil::IsEqualCaseInsen("BC5_SNORM", pStr))
			return Texturefmt::BC5_SNORM;
		if (core::strUtil::IsEqualCaseInsen("BC5_TYPELESS", pStr))
			return Texturefmt::BC5_TYPELESS;

		if (core::strUtil::IsEqualCaseInsen("BC6", pStr))
			return Texturefmt::BC6;
		if (core::strUtil::IsEqualCaseInsen("BC6_SF16", pStr))
			return Texturefmt::BC6_SF16;
		if (core::strUtil::IsEqualCaseInsen("BC6_TYPELESS", pStr))
			return Texturefmt::BC6_TYPELESS;


		if (core::strUtil::IsEqualCaseInsen("BC7", pStr))
			return Texturefmt::BC7;
		if (core::strUtil::IsEqualCaseInsen("BC7_SRGB", pStr))
			return Texturefmt::BC7_SRGB;
		if (core::strUtil::IsEqualCaseInsen("BC7_TYPELESS", pStr))
			return Texturefmt::BC7_TYPELESS;


		if (core::strUtil::IsEqualCaseInsen("R10G10B10A2", pStr))
			return Texturefmt::R10G10B10A2;

		if (core::strUtil::IsEqualCaseInsen("R16G16B16A16_FLOAT", pStr))
			return Texturefmt::R16G16B16A16_FLOAT;



		// this is here incase i've forgot to add a new format
		// into this function.
		X_ASSERT_UNREACHABLE();
		return Texturefmt::UNKNOWN;
	}


} // namespace Util

X_NAMESPACE_END
