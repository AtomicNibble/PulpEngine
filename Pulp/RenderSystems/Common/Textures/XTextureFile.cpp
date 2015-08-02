#include "stdafx.h"
#include "XTextureFile.h"

X_NAMESPACE_BEGIN(texture)


XTextureFile::XTextureFile() 
{
	core::zero_object(pFaces);

#if X_DEBUG
	pName = nullptr;
#endif //!X_DEBUG

	datasize = 0; 
	numMips = 0;
	depth = 0;	
	numFaces = 0;  
	bDontDelete = false;

	format = Texturefmt::UNKNOWN;
	type = TextureType::UNKNOWN;
}
XTextureFile::~XTextureFile() 
{
	for (int i = 0; i < 6; i++) {
		X_ASSERT(!(i > numFaces && pFaces[i]), "memory set on face out of range")(numFaces);
		if (!dontDelete()) {
			X_DELETE_ARRAY(pFaces[i], g_textureDataArena);
		}
	}
}


const bool XTextureFile::isValid(void) const
{
	// check every things been set.
	return size.x > 0 && size.y > 0 &&
		depth > 0 &&
		numMips > 0 &&
		datasize > 0 &&
		numFaces > 0 &&
		format != Texturefmt::UNKNOWN &&
		type != TextureType::UNKNOWN;
}

void XTextureFile::freeTextureFile(XTextureFile* image_file)
{
	X_ASSERT_NOT_NULL(image_file);

	X_DELETE(image_file, g_textureDataArena);
}

void XTextureFile::setNumFaces(const int faces)
{
	X_ASSERT(depth < std::numeric_limits<uint8_t>::max(), "invalid face count")(faces);
	this->numFaces = safe_static_cast<uint8_t, int>(faces);
}

void XTextureFile::setDepth(const int depth)
{ 
	X_ASSERT(depth < std::numeric_limits<uint8_t>::max(), "invalid depth")(depth);
	this->depth = safe_static_cast<uint8_t, int>(depth);
}

void XTextureFile::setNumMips(const int numMips)
{ 
	X_ASSERT(depth < std::numeric_limits<uint8_t>::max(), "invalid mipcount")(numMips);
	this->numMips = safe_static_cast<uint8_t, int>(numMips);
}

X_NAMESPACE_END


