#include "stdafx.h"
#include "XTextureFile.h"

X_NAMESPACE_BEGIN(texture)


XTextureFile::XTextureFile() 
{
	core::zero_this(this);

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

X_NAMESPACE_END
