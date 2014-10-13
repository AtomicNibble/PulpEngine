#include "stdafx.h"
#include "TextureLoaderPNG.h"

#include <String\StringUtil.h>
#include <IFileSys.h>

#include <Memory\MemCursor.h>
#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

#include <Containers\Array.h>

extern "C" {
#include "zlib\zlib.h"
}

#include "XTextureFile.h"

// link zlib plz.
#if X_DEBUG
X_LINK_LIB("zlibstatd");
#else
X_LINK_LIB("zlibstat");
#endif // !X_DEBUG

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

		static int png_get_bpp(uint32_t depth, uint32_t color_type)
		{
			int bpp;
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

		static const char* zlib_err_str(int z_err)
		{
			switch (z_err)
			{
			case Z_OK:
				return "no Error";
			case Z_STREAM_END:
				return "stream ended";
			case Z_NEED_DICT:
				return "need dict";
			case Z_ERRNO:
				return "ERRNO";
			case Z_STREAM_ERROR:
				return "stream error";
			case Z_DATA_ERROR:
				return "data error";
			case Z_MEM_ERROR:
				return "mem error";
			case Z_BUF_ERROR:
				return "buffer error";
			case Z_VERSION_ERROR:
				return "version error";

			default:
				break;
			}

			return "Unknown Error";
		}

		struct Png_Header
		{
			int64_t magic;

			bool isValid() const {
				return magic == PNG_FILE_MAGIC;
			}
		};

		void* StaticAlloc(void* opaque, uInt items, uInt size)
		{
			X_ASSERT_NOT_NULL(opaque);

			return ((core::MallocFreeAllocator*)opaque)->allocate(items * size, 4, 0);
		}

		void StaticFree(void* opaque, void* address)
		{
			X_ASSERT_NOT_NULL(opaque);
			X_ASSERT_NOT_NULL(address);

			((core::MallocFreeAllocator*)opaque)->free(address);
		}

		bool LoadChucksIDAT(core::XFile* file, uint32_t length, uint32_t InfaltedSize, uint8_t* inflated_data)
		{
			X_ASSERT_NOT_NULL(file);

			core::Array<uint8_t> ZlibData(g_rendererArena);
			ZlibData.reserve(length);

			bool	 valid = true;
			uint32_t tagName;
			uint32_t orig_crc;
			int32_t  z_result;

			z_stream stream;
			core::zero_object(stream);

			core::MallocFreeAllocator allocator;

			stream.next_in = (Bytef*)ZlibData.ptr();
			stream.avail_in = (uInt)length;

			stream.next_out = (Bytef*)inflated_data;
			stream.avail_out = (uInt)InfaltedSize;

			stream.zalloc = StaticAlloc;
			stream.zfree = StaticFree;
			stream.opaque = &allocator;

			::inflateInit(&stream);

			do
			{
				// fill the buffer with the data.
				if (file->read(ZlibData.ptr(), length) != length)
				{
					X_ERROR("TexturePNG", "failed to reed block of size: %i", length);
					valid = false;
					break;
				}

				// infalte it baby.
				z_result = inflate(&stream, Z_SYNC_FLUSH);

				// should we check 
				// stream.avail_in == 0 ? to make sure it was all eaten.

				if (z_result != Z_STREAM_END && z_result != Z_OK)
				{
					if (stream.msg != nullptr)
						X_ERROR("TexturePNG", "zlib error: %s(%i)", stream.msg, z_result);
					else
						X_ERROR("TexturePNG", "zlib error: %s", zlib_err_str(z_result));
					valid = false;
					break;
				}


				// after block there is a crc32
				file->readObj(orig_crc);

				size_t left = file->remainingBytes();

				if (file->readObj(length) != sizeof(length))
				{
					valid = false;
					break; // no more blocks.
				}

				// check for next tag
				if (file->readObj(tagName) != sizeof(tagName))
				{
					valid = false;
					break; // no more blocks.
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

				stream.next_in = (Bytef*)ZlibData.ptr();
				stream.avail_in = (uInt)length;

			} while (tagName == PNG_TAG_IDAT);


			// free any allotions the stream made.
			::inflateEnd(&stream);

			if (tagName != PNG_TAG_IEND) {
				X_WARNING("TexturePNG", "failed to find IEND tag");
			}

			return valid;
		}
	}

	XTexLoaderPNG::XTexLoaderPNG()
	{

	}

	XTexLoaderPNG::~XTexLoaderPNG()
	{

	}

	// ITextureLoader
	bool XTexLoaderPNG::canLoadFile(const core::Path& path) const
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


		if (height < 0 || height > TEX_MAX_DIMENSIONS || width < 0 || width > TEX_MAX_DIMENSIONS)
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
		uint8_t* inflated_data = X_NEW_ARRAY_ALIGNED(uint8_t,inflated_size,g_rendererArena,"PngFaceBuffer",8);


		if (!LoadChucksIDAT(file, length, inflated_size, inflated_data))
		{
			X_ERROR("TexturePNG", "failed to load PNG chunks");
			X_DELETE_ARRAY( inflated_data, g_rendererArena);
			return nullptr;
		}

		// inflated_data is the uncompressed image data.
		// make img info.
		XTextureFile* img = X_NEW_ALIGNED(XTextureFile, g_rendererArena, "TextureFile", 8);
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
		img->setHeigth(height);
		img->setWidth(width);
		img->setDataSize(inflated_size);

		return img;
	}

	// ~ITextureLoader




} // namespace PNG

X_NAMESPACE_END