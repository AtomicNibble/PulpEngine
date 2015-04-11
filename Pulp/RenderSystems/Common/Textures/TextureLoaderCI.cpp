#include "stdafx.h"
#include "TextureLoaderCI.h"

#include <String\StringUtil.h>
#include <IFileSys.h>
#include <ITexture.h>

#include "XTextureFile.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{
	namespace
	{
		static const char*		CI_FILE_EXTENSION = ".ci";
		static const uint32_t	CI_FOURCC = X_TAG('c', 'i', 'm', 'g');
		static const uint32_t	CI_VERSION = 2;
		// these are format limits.
		// global limtis must still be respected when loading.
		static const uint32_t	CI_MAX_DIMENSIONS = UINT16_MAX;
		static const uint32_t	CI_MAX_MIPS = UINT8_MAX;
		static const uint32_t	CI_MAX_FACES = UINT8_MAX;


		struct TexureHeader
		{
			TexureHeader() {
				core::zero_this(this);
			}

			uint32 fourCC;
			uint8 version;
			Texturefmt::Enum format;
			uint8 Mips;
			uint8 Faces;

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

		// add a static assert here that TEX_MAX_DIMENSIONS is not bigger than what this format
		// can store.
		X_ENSURE_LE(TEX_MAX_DIMENSIONS, CI_MAX_DIMENSIONS, "TEX_MAX_DIMENSIONS exceeds what CI image can store");
		X_ENSURE_LE(TEX_MAX_MIPS, CI_MAX_MIPS, "TEX_MAX_MIPS exceeds what CI image can store");
		X_ENSURE_LE(TEX_MAX_FACES, CI_MAX_FACES, "TEX_MAX_FACES exceeds what CI image can store");
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
		if (hdr.height > TEX_MAX_DIMENSIONS || hdr.width > TEX_MAX_DIMENSIONS)
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


		X_ASSERT_NOT_IMPLEMENTED();

		return nullptr;
	}
	// ~ITextureLoader


} // namespace CI

X_NAMESPACE_END