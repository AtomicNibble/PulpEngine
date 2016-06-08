#include "stdafx.h"
#include "TextureLoaderTGA.h"

#include <IFileSys.h>


#include "XTextureFile.h"

X_NAMESPACE_BEGIN(texture)

namespace TGA
{
	namespace
	{
		static const char* TGA_FILE_EXTENSION = ".tga";


		X_PRAGMA(pack(push,1))
		struct Tga_Header
		{
			uint32_t		IDLength;        /* 00h  Size of Image ID field */
			uint32_t		ColorMapType;    /* 01h  Color map type */
			uint32_t		ImageType;       /* 02h  Image type code */
			uint32_t		CMapStart;       /* 03h  Color map origin */
			uint32_t		CMapLength;      /* 05h  Color map length */
			uint32_t		CMapDepth;       /* 07h  Depth of color map entries */
			uint32_t		XOffset;         /* 08h  X origin of image */
			uint32_t		YOffset;         /* 0Ah  Y origin of image */
			uint32_t		Width;           /* 0Ch  Width of image */
			uint32_t		Height;          /* 0Eh  Height of image */
			uint32_t		PixelDepth;      /* 10h  Image pixel size */
			uint32_t		ImageDescriptor; /* 11h  Image descriptor byte */
		};
		X_PRAGMA(pack(pop))

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



XTexLoaderTGA::XTexLoaderTGA()
{

}

XTexLoaderTGA::~XTexLoaderTGA()
{

}

// ITextureLoader
bool XTexLoaderTGA::canLoadFile(const core::Path<char>& path) const
{
	return  core::strUtil::IsEqual(TGA_FILE_EXTENSION, path.extension());
}

XTextureFile* XTexLoaderTGA::loadTexture(core::XFile* file)
{
	X_ASSERT_NOT_NULL(file);

	Tga_Header hdr;
	uint8_t buf[18];

	if (file->readObj(buf) != sizeof(buf)) {
		X_ERROR("Tga", "Failed to read header");
		return nullptr;
	}

	hdr.IDLength = (uint32_t)buf[0];
	hdr.ColorMapType = (uint32_t)buf[1];
	hdr.ImageType = (uint32_t)buf[2];
	hdr.CMapStart = (uint32_t)buf[3] | (((uint32_t)buf[4]) << 8);
	hdr.CMapLength = (uint32_t)buf[5] | (((uint32_t)buf[6]) << 8);
	hdr.CMapDepth = (uint32_t)buf[7];
	hdr.XOffset = (uint32_t)buf[8] | (((uint32_t)buf[9]) << 8);
	hdr.YOffset = (uint32_t)buf[10] | (((uint32_t)buf[11]) << 8);
	hdr.Width = (uint32_t)buf[12] | (((uint32_t)buf[13]) << 8);
	hdr.Height = (uint32_t)buf[14] | (((uint32_t)buf[15]) << 8);
	hdr.PixelDepth = (uint32_t)buf[16];
	hdr.ImageDescriptor = (uint32_t)buf[17];


	// Validate TGA header (is this a TGA file?)
	if (hdr.ColorMapType != 0 && hdr.ColorMapType != 1)
	{
		X_ERROR("TextureTGA", "invalid color map type. provided: %i expected: 0 | 1", hdr.ColorMapType);
		return nullptr;
	}

	if (!isValidImageType(hdr.ImageType))
	{
		X_ERROR("TextureTGA", "invalid image type. provided: %i expected: 1-3 | 9-11", hdr.ImageType);
		return nullptr;
	}

	if (!isColorMap(hdr.ImageType))
	{
		X_ERROR("TextureTGA", "invalid image type. only color maps allowed", hdr.ImageType);
		return nullptr;
	}

	if (isRle(hdr.ImageType))
	{
		X_ERROR("TextureTGA", "rle images are not supported", hdr.ImageType);
		return nullptr;
	}

	if (isRightToLeft(hdr.ImageDescriptor))
	{
		X_ERROR("TextureTGA", "right to left images are not supported", hdr.ImageType);
		return nullptr;
	}

	if (isTopToBottom(hdr.ImageDescriptor))
	{
		X_ERROR("TextureTGA", "top to bottom images are not supported", hdr.ImageType);
		return nullptr;
	}


	if (!(hdr.PixelDepth == 8 || hdr.PixelDepth == 24 || hdr.PixelDepth == 32))
	{
		X_ERROR("TextureTGA", "invalid pixeldepth. provided: %i expected: 8 | 24 | 32", hdr.PixelDepth);
		return nullptr;
	}

	if (hdr.Height < 1 || hdr.Height > TEX_MAX_DIMENSIONS || hdr.Width < 1 || hdr.Width > TEX_MAX_DIMENSIONS)
	{
		X_ERROR("TextureTGA", "invalid image dimensions. provided: %ix%i max: %ix%i", hdr.Height, hdr.Width, TEX_MAX_DIMENSIONS, TEX_MAX_DIMENSIONS);
		return nullptr;
	}

	if (!core::bitUtil::IsPowerOfTwo(hdr.Height) || !core::bitUtil::IsPowerOfTwo(hdr.Width))
	{
		X_ERROR("TextureTGA", "invalid image dimensions, must be power of two. provided: %ix%i", hdr.Height, hdr.Width);
		return nullptr;
	}

	// load the data.
	XTextureFile* img = X_NEW_ALIGNED(XTextureFile, g_textureDataArena, "TextureFile", 8);
	TextureFlags flags;
	flags.Set(TextureFlags::NOMIPS);
	flags.Set(TextureFlags::ALPHA);

	uint32_t DataSize = hdr.Width * hdr.Height * (hdr.PixelDepth / 8);

	img->pFaces[0] = X_NEW_ARRAY_ALIGNED(uint8_t, DataSize, g_textureDataArena, "TgaFaceBuffer", 8);
	img->setDataSize(DataSize);
	img->setWidth(safe_static_cast<uint16_t, uint32_t>(hdr.Width));
	img->setHeigth(safe_static_cast<uint16_t, uint32_t>(hdr.Height));
	img->setNumFaces(1);
	img->setDepth(1);
	img->setNumMips(1);
	img->setType(TextureType::T2D);

	switch (hdr.PixelDepth)
	{
	case 8:
		img->setFormat(Texturefmt::A8);
		break;
	case 24:
		img->setFormat(Texturefmt::R8G8B8);
		break;
	case 32:
		img->setFormat(Texturefmt::R8G8B8A8);
		break;
	}

	// read the image data.

	size_t bytes_read = file->read(img->pFaces[0], DataSize);

	if (bytes_read != DataSize)
	{
		X_ERROR("TextureTGA", "failed to read image data from. requested: %i bytes recivied: %i bytes",
			DataSize, bytes_read);

		X_DELETE(img, g_textureDataArena);
		img = nullptr;
	}

#if X_DEBUG == 1
	uint64_t left = file->remainingBytes();
	X_WARNING_IF(left > 0, "TextureTGA", "potential read fail, bytes left in file: %i", left);
#endif

	return img;
}

// ~ITextureLoader


bool XTexLoaderTGA::isValidImageType(uint32_t type)
{
	switch (type)
	{
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

	switch (type)
	{
	case ImageType::COLORMAP:
	case ImageType::COLORMAP_RLE:
		return true;

	default:
		break;
	}
	return false;
}

bool XTexLoaderTGA::isMono(uint32_t type)
{
	X_ASSERT(isValidImageType(type), "Invalid format passed")();

	switch (type)
	{
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

	switch (type)
	{
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
	return core::bitUtil::IsBitSet( descriptor, 4);
}

bool XTexLoaderTGA::isTopToBottom(uint32_t descriptor)
{
	return core::bitUtil::IsBitSet(descriptor, 5);
}



} // namespace TGA

X_NAMESPACE_END