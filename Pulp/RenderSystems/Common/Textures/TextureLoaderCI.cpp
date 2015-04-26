#include "stdafx.h"
#include "TextureLoaderCI.h"

#include <String\StringUtil.h>
#include <IFileSys.h>
#include <ITexture.h>
#include <ICi.h>

#include "XTextureFile.h"

X_NAMESPACE_BEGIN(texture)

namespace CI
{
	namespace
	{
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

		CITexureHeader hdr;

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

		if ((hdr.DataSize % hdr.FaceSize) != 0)
		{
			X_ERROR("TextureCI", "data size is not a multiple of facesize");
			return nullptr;
		}

		uint8_t* pImgBuf = X_NEW_ARRAY_ALIGNED(uint8_t, hdr.DataSize, g_textureDataArena, "CIImgBuffer", 8);
		uint32_t bytesRead = file->read(pImgBuf, hdr.DataSize);

		if (bytesRead != hdr.DataSize)
		{
			X_ERROR("TextureCI", "failed to read image data from CIImage. got: %x wanted: %x bytes",
				bytesRead, hdr.DataSize);

			X_DELETE_ARRAY(pImgBuf, g_textureDataArena);
			return nullptr;
		}

		if (file->remainingBytes() != 0)
		{
			X_ERROR("TextureCI", "read fail, bytes left in file: %i", file->remainingBytes());
			return nullptr;
		}

		// Load the data.
		XTextureFile* img = X_NEW(XTextureFile, g_textureDataArena, "TextureFile");

		// flags
		TextureFlags flags = hdr.Flags;
		// tools not required to set this
		// so just set it here so it's reliable.
		// since this is the only place it matters.
		flags.Set(TexFlag::CI_IMG);


		// set the info
		img->setWidth(hdr.width);
		img->setHeigth(hdr.height);
		img->setNumMips(hdr.Mips);
		img->setNumFaces(hdr.Faces); // 1 for 2D 6 for a cube.
		img->setDepth(1); /// We Don't allow volume texture loading yet.
		img->setFlags(flags);
		img->setFormat(hdr.format);

		uint32_t offset = 0;
		for (uint32_t i = 0; i < hdr.Faces; i++)
		{
			img->pFaces[i] = &pImgBuf[offset];

			// add offset.
			offset += hdr.FaceSize;
		}

		// check offfset is same as size.
		if (offset != hdr.DataSize) {
			X_ERROR("TextureCI", "error setting face pointers, "
				"img data is not equal to expected face size. Delta: %i",
				hdr.DataSize - offset);
			return nullptr;
		}

		return img;
	}
	// ~ITextureLoader


} // namespace CI

X_NAMESPACE_END