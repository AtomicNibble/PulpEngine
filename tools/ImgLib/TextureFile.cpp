#include "stdafx.h"
#include "TextureFile.h"


X_NAMESPACE_BEGIN(texture)

XTextureFile::XTextureFile(core::MemoryArenaBase* arena) :
	data_(arena)
{
	core::zero_object(mipOffsets_);
	core::zero_object(faceOffsets_);

	format_ = Texturefmt::UNKNOWN;
	type_ = TextureType::UNKNOWN;

	datasize_ = 0;
	numMips_ = 0;
	depth_ = 0;
	numFaces_ = 0;

	sizeValid_ = false;
}

XTextureFile::~XTextureFile()
{

}

void XTextureFile::resize(void)
{
	X_ASSERT_NOT_IMPLEMENTED();

}

void XTextureFile::clear(void)
{
	core::zero_object(mipOffsets_);
	core::zero_object(faceOffsets_);

	data_.clear();
}

void XTextureFile::free(void)
{
	clear();

	data_.free();
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

void XTextureFile::setNumFaces(const int32_t faces)
{
	X_ASSERT(depth_ < std::numeric_limits<uint8_t>::max(), "invalid face count")(faces);
	numFaces_ = safe_static_cast<uint8_t, int32_t>(faces);
}

void XTextureFile::setDepth(const int32_t depth)
{
	X_ASSERT(depth < std::numeric_limits<uint8_t>::max(), "invalid depth")(depth);
	depth_ = safe_static_cast<uint8_t, int32_t>(depth);
}

void XTextureFile::setNumMips(const int32_t numMips)
{
	X_ASSERT(depth_ < std::numeric_limits<uint8_t>::max(), "invalid mipcount")(numMips);
	numMips_ = safe_static_cast<uint8_t, int32_t>(numMips);
}



X_NAMESPACE_END
