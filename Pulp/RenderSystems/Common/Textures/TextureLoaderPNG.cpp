#include "stdafx.h"
#include "TextureLoaderPNG.h"

#include <String\StringUtil.h>
#include <IFileSys.h>

#include <Memory\MemCursor.h>
#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

#include <Containers\Array.h>

#include <Compression\Zlib.h>

#include "XTextureFile.h"


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

		
		struct PngColorType
		{
			enum Enum
			{
				GREYSCALE = 0,
				TRUECOLOR = 2,
				INDEXED = 3,
				GREYSCALE_ALPHA = 4,
				TRUECOLOR_ALPHA = 6
			};
		};

		static uint32_t png_get_bpp(uint32_t depth, uint32_t color_type)
		{
			uint32_t bpp;
			switch (color_type)
			{
			case PngColorType::GREYSCALE:
				bpp = 1; break;
			case PngColorType::TRUECOLOR:
				bpp = 3; break;
			case PngColorType::INDEXED:
				bpp = 1; break;
			case PngColorType::GREYSCALE_ALPHA:
				bpp = 2; break;
			case PngColorType::TRUECOLOR_ALPHA:
				bpp = 4; break;
			default:
				X_ASSERT_UNREACHABLE();
				return 0;
			}
			bpp *= depth / 8;
			return bpp;
		}

		static Texturefmt::Enum map_png_format(uint32_t color_type)
		{
			switch (color_type)
			{
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

			bool isValid() const {
				return magic == PNG_FILE_MAGIC;
			}
		};


		bool LoadChucksIDAT(core::XFile* file, uint32_t length, uint32_t InfaltedSize, uint8_t* inflated_data)
		{
			X_ASSERT_NOT_NULL(file);

			core::Array<uint8_t> ZlibData(g_textureDataArena);
			ZlibData.reserve(length);

			uint32_t tagName;
			uint32_t orig_crc;

			using namespace core::Compression;

			ZlibInflate inflater(inflated_data, InfaltedSize);
			ZlibInflate::InflateResult::Enum zRes;

			do
			{
				// fill the buffer with the data.
				if (file->read(ZlibData.ptr(), length) != length)
				{
					X_ERROR("TexturePNG", "failed to reed block of size: %i", length);
					return false;
				}

				// infalte it baby.
				zRes = inflater.Inflate(ZlibData.ptr(), length);

				if (zRes == ZlibInflate::InflateResult::ERROR)
				{
					X_ERROR("TexturePNG", "Zlib error");
					return false;
				}

				// after block there is a crc32
				file->readObj(orig_crc);

			//	size_t left = file->remainingBytes();

				if (file->readObj(length) != sizeof(length))
				{
					X_ERROR("TexturePNG", "failed to read tag length");
					return false;
				}

				// check for next tag
				if (file->readObj(tagName) != sizeof(tagName))
				{
					X_ERROR("TexturePNG", "failed to read tag name");
					return false;
				}

				length = core::Endian::swap(length);

				// ok another block if the block size is bigger we can either.
				// 1. do multiple reads and inflates
				// 2. reallocate buffer.
				// from reading specs most software writing multiple IDAT chucks is for a fixed buffer
				// size, so it's unlikley the next block will be bigger, so just going to reallocate if needed
				// to avoid adding additional complexity to the loop.

				if (length > safe_static_cast<uint32_t, size_t>(ZlibData.capacity()))
				{
					ZlibData.reserve(length);
				}

			} while (tagName == PNG_TAG_IDAT);

			if (zRes != ZlibInflate::InflateResult::DONE) {
				X_ERROR("TexturePNG", "Potential zlib error, did not inflate expected amount");
				return false;
			}

			if (tagName != PNG_TAG_IEND) {
				X_WARNING("TexturePNG", "failed to find IEND tag");
			}

			return true;
		}
	}

	XTexLoaderPNG::XTexLoaderPNG()
	{

	}

	XTexLoaderPNG::~XTexLoaderPNG()
	{

	}

	// ITextureLoader
	bool XTexLoaderPNG::canLoadFile(const core::Path<char>& path) const
	{
		return  core::strUtil::IsEqual(PNG_FILE_EXTENSION, path.extension());
	}

	XTextureFile* XTexLoaderPNG::loadTexture(core::XFile* file)
	{
		X_ASSERT_NOT_NULL(file);

		Png_Header hdr;
		uint32_t length, i, tagName;
		uint32_t orig_crc, width, height, depth, color_type;
		uint32_t compression_method, filter_method, interlace_method, bpp;
		uint8_t ihdr[13 + 4];	/* length should be 13, make room for type (IHDR) */

		file->readObj(hdr);

		if (!hdr.isValid())
		{
			X_ERROR("TexturePNG", "invalid png header.");
			return nullptr;
		}

		file->readObj(length);
		length = core::Endian::swap(length);

		if (length != 13)
		{
			X_ERROR("TexturePNG", "invalid png length. provided: %i required: 13", length);
			return nullptr;
		}

		file->readObj(ihdr);
		file->readObj(orig_crc);

		core::MemCursor<uint8_t> cursor(ihdr, 13+4);
	
		if (cursor.getSeek<uint32_t>() != PNG_TAG_IHDR)
		{
			X_ERROR("TexturePNG", "invalid sub tag. expoected: IHDR");
			return nullptr;
		}

		width = core::Endian::swap(cursor.getSeek<uint32_t>());
		height = core::Endian::swap(cursor.getSeek<uint32_t>());

		depth = cursor.getSeek<uint8_t>();
		color_type = cursor.getSeek<uint8_t>();
		compression_method = cursor.getSeek<uint8_t>();
		filter_method = cursor.getSeek<uint8_t>();
		interlace_method = cursor.getSeek<uint8_t>();


		if (height < 1 || height > TEX_MAX_DIMENSIONS || width < 1 || width > TEX_MAX_DIMENSIONS)
		{
			X_ERROR("TexturePNG", "invalid image dimensions. provided: %ix%i max: %ix%i", 
				height, width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);
			return nullptr;
		}

		if (!core::bitUtil::IsPowerOfTwo(height) || !core::bitUtil::IsPowerOfTwo(width))
		{
			X_ERROR("TexturePNG", "invalid image dimensions, must be power of two. provided: %ix%i", 
				height, width);
			return nullptr;
		}

		if (color_type == PngColorType::INDEXED)
		{
			X_ERROR("TexturePNG", "invalud color type. expected: TRUECOLOR | TRUECOLOR_ALPHA");
			return nullptr;
		}

		if (depth != 8 && depth != 16)
		{
			X_ERROR("TexturePNG", "invalud depth. provided: %i expected: 8 | 16", depth);
			return nullptr;
		}
	
		if (interlace_method > 0)
		{
			X_ERROR("TexturePNG", "interlace not supported");
			return nullptr;
		}
		
		bpp = png_get_bpp(depth,color_type);

		// read any more tags!
		for (i = 0; i < 32; i++)
		{
			if (file->readObj(length) != sizeof(length)) {
				X_ERROR("TexturePNG", "failed to read TAG length");
				return nullptr;
			}

			length = core::Endian::swap(length);
			
			// read the Tag name
			file->readObj(tagName);

			if (tagName == PNG_TAG_IDAT) {
				break;
			}

			// skip length + crc
			file->seek(length + 4, core::SeekMode::CUR);
		}

		if (tagName != PNG_TAG_IDAT) {
			X_ERROR("TexturePNG", "failed to find IDAT tag in file");
			return nullptr;
		}

		// ok so the length is the size of the compreseed block we need to read.
		// it is possible to have multiple IDAT blocks.
		uint32_t inflated_size = (width * height * bpp);
		uint8_t* inflated_data = X_NEW_ARRAY_ALIGNED(uint8_t, inflated_size, g_textureDataArena, "PngFaceBuffer", 8);


		if (!LoadChucksIDAT(file, length, inflated_size, inflated_data))
		{
			X_ERROR("TexturePNG", "failed to load PNG chunks");
			X_DELETE_ARRAY(inflated_data, g_textureDataArena);
			return nullptr;
		}

		// inflated_data is the uncompressed image data.
		// make img info.
		XTextureFile* img = X_NEW_ALIGNED(XTextureFile, g_textureDataArena, "TextureFile", 8);
		TextureFlags flags;
		flags.Set(TextureFlags::NOMIPS);

		if (color_type == PngColorType::TRUECOLOR_ALPHA)
			flags.Set(TextureFlags::ALPHA);

		img->setFormat(map_png_format(color_type));

		img->pFaces[0] = inflated_data;
		img->setNumFaces(1);
		img->setNumMips(1);
		img->setDepth(1);
		img->setFlags(flags);
		img->setType(TextureType::T2D);
		img->setHeigth(safe_static_cast<uint16_t,uint32_t>(height));
		img->setWidth(safe_static_cast<uint16_t, uint32_t>(width));
		img->setDataSize(inflated_size);

		return img;
	}

	// ~ITextureLoader




} // namespace PNG

X_NAMESPACE_END