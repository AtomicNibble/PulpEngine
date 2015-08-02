#pragma once


#ifndef _X_TEXTURE_FILE_H_
#define _X_TEXTURE_FILE_H_

#include "Util\ReferenceCounted.h"

X_NAMESPACE_BEGIN(texture)


// Texture loaders return this information.
// it is used by image stream / file loaders to pass back
// loaded image files to Xtexture which can then be uploaded to gpu / processed.
// It's basically a uniformed way of sending back the image data.
// no matter what file format it came from.

namespace DDS {
	class XTexLoaderDDS;
}
namespace JPG {
	class XTexLoaderJPG;
}
namespace PNG {
	class XTexLoaderPNG;
}
namespace PSD {
	class XTexLoaderPSD;
}
namespace TGA {
	class XTexLoaderTGA;
}
namespace CI {
	class XTexLoaderCI;
}
/*

Vec2<uint16_t>		size;
uint32_t			datasize; // size of whole image / 1 face.
TextureType::Enum	type;
TextureFlags		flags;
Texturefmt::Enum 	format;
uint8_t				numMips;
uint8_t				depth;	// Volume x,y,w
uint8_t				numFaces;  // Cube maps aka 6 faces.

*/

struct XTextureFile : public core::ReferenceCounted<XTextureFile>
{
	XTextureFile();
	~XTextureFile();

	friend class DDS::XTexLoaderDDS;
	friend class JPG::XTexLoaderJPG;
	friend class PNG::XTexLoaderPNG;
	friend class TGA::XTexLoaderTGA;
	friend class PSD::XTexLoaderPSD;
	friend class CI::XTexLoaderCI;

	friend class XTexture;


public:

#if X_DEBUG
	X_INLINE const char* getName(void) const { return pName; }
#endif // !X_DEBUG

	X_INLINE const Vec2<uint16_t> getSize(void) const { return size; }
	X_INLINE const int getWidth(void) const { return size.x; }
	X_INLINE const int getHeight(void) const { return size.y; }
	X_INLINE const int getNumFaces(void) const { return numFaces; }
	X_INLINE const int getDepth(void) const { return depth; }
	X_INLINE const int getNumMips(void) const { return numMips; }
	X_INLINE const uint32_t getDataSize(void) const { return datasize; }
	X_INLINE TextureFlags getFlags(void) const { return flags; }
	X_INLINE Texturefmt::Enum getFormat(void) const { return format; }
	X_INLINE TextureType::Enum getType(void) const { return type; }

	const bool isValid(void) const;

	struct MipInfo
	{
		MipInfo() : 
			pSysMem(nullptr), 
			SysMemPitch(0), 
			SysMemSlicePitch(0) {}

		const void* pSysMem;
		uint32_t    SysMemPitch;
		uint32_t    SysMemSlicePitch;
	};

	MipInfo SubInfo[TEX_MAX_MIPS];

	// the data, 6 for cube maps :)
	uint8_t* pFaces[6];

public:
	// used by UT.
	static void freeTextureFile(XTextureFile* image_file);

	// we have setters, for the loaders.
protected:
	X_INLINE void setSize(const Vec2<uint16_t> size) { this->size = size; }
	X_INLINE void setWidth(const uint16_t width) { size.x = width; }
	X_INLINE void setHeigth(const uint16_t height) { size.y = height; }
	void setNumFaces(const int num);
	void setDepth(const int depth);
	void setNumMips(const int num);
	X_INLINE void setFlags(TextureFlags	flags) { this->flags = flags; }
	X_INLINE void setType(TextureType::Enum type) { this->type = type; }
	X_INLINE void setFormat(Texturefmt::Enum format) { this->format = format; }
	X_INLINE void setDataSize(const uint32_t num_bytes) { this->datasize = num_bytes; }

	X_INLINE bool dontDelete(void) const { return bDontDelete > 0; }


private:
#if X_DEBUG
	const char*			pName;
#endif // !X_DEBUG

	Vec2<uint16_t>		size;
	uint32_t			datasize; // size of whole image / 1 face.
	TextureType::Enum	type;
	TextureFlags		flags;
	Texturefmt::Enum 	format;
	uint8_t				numMips;
	uint8_t				depth;	// Volume x,y,w
	uint8_t				numFaces;  // Cube maps aka 6 faces.
	uint8_t				bDontDelete;
};


X_NAMESPACE_END


#endif // _X_TEXTURE_FILE_H_