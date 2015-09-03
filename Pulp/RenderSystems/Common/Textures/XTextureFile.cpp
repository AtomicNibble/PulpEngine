#include "stdafx.h"
#include "XTextureFile.h"

X_NAMESPACE_BEGIN(texture)


XTextureFile::XTextureFile() 
{
	core::zero_object(pFaces);

#if X_DEBUG
	pName_ = nullptr;
#endif //!X_DEBUG

	datasize_ = 0; 
	numMips_ = 0;
	depth_ = 0;	
	numFaces_ = 0;  
	bDontDelete_ = false;

	format_ = Texturefmt::UNKNOWN;
	type_ = TextureType::UNKNOWN;
}
XTextureFile::~XTextureFile() 
{
	for (int i = 0; i < 6; i++) {
		X_ASSERT(!(i > numFaces_ && pFaces[i]), "memory set on face out of range")(numFaces_);
		if (!dontDelete()) {
			X_DELETE_ARRAY(pFaces[i], g_textureDataArena);
		}
	}
}


const bool XTextureFile::isValid(void) const
{
	// check every things been set.
	return size_.x > 0 && size_.y > 0 &&
		depth_ > 0 &&
		numMips_ > 0 &&
		datasize_ > 0 &&
		numFaces_ > 0 &&
		format_ != Texturefmt::UNKNOWN &&
		type_ != TextureType::UNKNOWN;
}

void XTextureFile::freeTextureFile(XTextureFile* image_file)
{
	X_ASSERT_NOT_NULL(image_file);

	X_DELETE(image_file, g_textureDataArena);
}

void XTextureFile::setNumFaces(const int faces)
{
	X_ASSERT(depth_ < std::numeric_limits<uint8_t>::max(), "invalid face count")(faces);
	this->numFaces_ = safe_static_cast<uint8_t, int>(faces);
}

void XTextureFile::setDepth(const int depth)
{ 
	X_ASSERT(depth < std::numeric_limits<uint8_t>::max(), "invalid depth")(depth);
	this->depth_ = safe_static_cast<uint8_t, int>(depth);
}

void XTextureFile::setNumMips(const int numMips)
{ 
	X_ASSERT(depth_ < std::numeric_limits<uint8_t>::max(), "invalid mipcount")(numMips);
	this->numMips_ = safe_static_cast<uint8_t, int>(numMips);
}

X_NAMESPACE_END


