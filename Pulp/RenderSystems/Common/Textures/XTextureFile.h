#pragma once


#ifndef _X_TEXTURE_FILE_H_
#define _X_TEXTURE_FILE_H_

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

struct XTextureFile
{
	XTextureFile() {
		core::zero_this(this);

		format = Texturefmt::UNKNOWN;
		type = TextureType::UNKNOWN;
	}
	~XTextureFile() {
		for (int i = 0; i < 6; i++) {
			X_ASSERT(!(i > numFaces && pFaces[i]), "memory set on face out of range")(numFaces);
			if (!dontDelete())
				X_DELETE_ARRAY( pFaces[i], g_rendererArena);
		}
	}

	friend class DDS::XTexLoaderDDS;
	friend class JPG::XTexLoaderJPG;
	friend class PNG::XTexLoaderPNG;
	friend class TGA::XTexLoaderTGA;
	friend class PSD::XTexLoaderPSD;

	friend class XTexture;


public:
#if X_DEBUG
	X_INLINE const char* getName(void) const { return pName; }
#endif // !X_DEBUG

	X_INLINE const Vec2<uint16_t> getSize() const { return size; }
	X_INLINE const int getWidth() const { return size.x; }
	X_INLINE const int getHeight() const { return size.y; }
	X_INLINE const int getNumFaces() const { return numFaces; }
	X_INLINE const int getDepth() const { return depth; }
	X_INLINE const int getNumMips() const { return numMips; }
	X_INLINE const int getDataSize() const { return datasize; }
	X_INLINE TextureFlags getFlags() const { return flags; }
	X_INLINE Texturefmt::Enum getFormat() const { return format; }
	X_INLINE TextureType::Enum getType() const { return type; }

	X_INLINE const bool isValid() const {
		// check every things been set.
		return size.x > 0 && size.y > 0 &&
			depth > 0 &&
			numMips > 0 &&
			datasize > 0 &&
			numFaces > 0 &&
			format != Texturefmt::UNKNOWN &&
			type != TextureType::UNKNOWN;
	}

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

	// we have setters, for the loaders.
protected:
	X_INLINE void setSize(const Vec2<uint16_t> size) { this->size = size; }
	X_INLINE void setWidth(const int width) { size.x = width; }
	X_INLINE void setHeigth(const int height) { size.y = height; }
	X_INLINE void setNumFaces(const int num) { this->numFaces = num; }
	X_INLINE void setDepth(const int depth) { this->depth = depth; }
	X_INLINE void setNumMips(const int num) { this->numMips = num; }
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