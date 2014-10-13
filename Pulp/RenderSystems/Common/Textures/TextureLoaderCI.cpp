#include "stdafx.h"
#include "TextureLoaderCI.h"

#include <String\StringUtil.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(texture)

namespace CI
{
	namespace
	{
		static const char*		CI_FILE_EXTENSION = ".ci";
		static const uint32_t	CI_FOURCC = X_TAG('c', 'i', 'm', 'g');
		static const uint32_t	CI_VERSION = 2;


		struct TexureHeader
		{
			TexureHeader() {
				core::zero_this(this);
			}

			uint32 fourCC;
			uint8 version;
			uint8 format;
			uint8 Mips;
			uint8 _unused;
			TextureFlags Flags;

			union {
				struct {
					uint16_t width;
					uint16_t height;
				};
				struct {
					Vec2<uint16_t> size;
				};
			};
			uint32 DataSize;
			uint32 __Unused[4]; // room for expansion.

			bool isValid(void) const {
				return fourCC == CI_FOURCC;
			}
		};

		X_ENSURE_SIZE(TexureHeader, 36);
	}

	XTexLoaderCI::XTexLoaderCI()
	{

	}

	XTexLoaderCI::~XTexLoaderCI()
	{

	}

	// ITextureLoader
	bool XTexLoaderCI::canLoadFile(const core::Path& path) const
	{
		return  core::strUtil::IsEqual(CI_FILE_EXTENSION, path.extension());
	}

	XTextureFile* XTexLoaderCI::loadTexture(core::XFile* file)
	{
		X_ASSERT_NOT_NULL(file);

		TexureHeader hdr;

		// file system will report read error in log.
		// but lets return here also.
		if (file->readObj(hdr) != sizeof(hdr))
			return nullptr;
		
		if (hdr.isValid()) {
			X_ERROR("TextureCI", "header is not valid");
			return nullptr;
		}

		if (hdr.version != CI_VERSION) {
			X_ERROR("TextureCI", "version is invalid. provided: %i expected: %i", hdr.version, CI_VERSION);
			return nullptr;
		}

		// ok that's all the checks we need to do
		// since image has passed all the required checks to become a CI.
		// but if someone decides to make a tool that can make CI.
		// they might try load a larger image, so lets just check dimensions.
		if (hdr.height < 0 || hdr.height > TEX_MAX_DIMENSIONS || hdr.width < 0 || hdr.width > TEX_MAX_DIMENSIONS)
		{
			X_ERROR("TextureCI", "invalid image dimensions. provided: %ix%i max: %ix%i", 
				hdr.height, hdr.width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);
			return nullptr;
		}

		if (!core::bitUtil::IsPowerOfTwo(hdr.height) || !core::bitUtil::IsPowerOfTwo(hdr.width))
		{
			X_ERROR("TextureCI", "invalid image dimensions, must be power of two. provided: %ix%i", 
				hdr.height, hdr.width);
			return nullptr;
		}


		// Load the data.




		return nullptr;
	}
	// ~ITextureLoader


} // namespace CI

X_NAMESPACE_END