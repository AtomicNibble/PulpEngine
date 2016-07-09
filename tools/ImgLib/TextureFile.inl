

X_NAMESPACE_BEGIN(texture)


X_INLINE XTextureFile::XTextureFile(core::MemoryArenaBase* arena) :
	data_(arena)
{
	core::zero_object(mipOffsets_);
	core::zero_object(faceOffsets_);

	format_ = Texturefmt::UNKNOWN;
	type_ = TextureType::UNKNOWN;

	numMips_ = 0;
	depth_ = 0;
	numFaces_ = 0;

	sizeValid_ = false;
}

X_INLINE XTextureFile::~XTextureFile()
{

}

X_INLINE void XTextureFile::resize(void)
{
	X_ASSERT(format_ != Texturefmt::UNKNOWN, "format must be set")(format_);

	// work out the size needed.
	const uint32_t faceSize = Util::dataSize(size_.x, size_.y, numMips_, format_);
	const uint32_t requiredBytes = faceSize * numFaces_;

	if (depth_ > 1) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	data_.resize(requiredBytes);

	mipOffsets_[0] = 0;
	faceOffsets_[0] = 0;


	{
		uint32_t width = core::Max<uint32_t>(1u, size_.x);
		uint32_t height = core::Max<uint32_t>(1u, size_.y);

		// 0 is higest.
		for (uint32_t i = 1; i < numMips_; i++)
		{
			mipOffsets_[i] = (mipOffsets_[i - 1] + Util::dataSize(width, height, depth_, format_));

			width >>= 1;
			height >>= 1;
		}
	}

	for (uint32_t i = 0; i < numFaces_; i++)
	{

	}
}

X_INLINE void XTextureFile::clear(void)
{
	core::zero_object(mipOffsets_);
	core::zero_object(faceOffsets_);

	data_.clear();
}

X_INLINE void XTextureFile::free(void)
{
	clear();

	data_.free();
}


X_INLINE const bool XTextureFile::isValid(void) const
{
	// check every things been set.
	return size_.x > 0 && size_.y > 0 &&
		depth_ > 0 &&
		numMips_ > 0 &&
		data_.isNotEmpty() &&
		numFaces_ > 0 &&
		format_ != Texturefmt::UNKNOWN &&
		type_ != TextureType::UNKNOWN;
}


X_INLINE const Vec2<uint16_t>& XTextureFile::getSize(void) const
{ 
	return size_; 
}

X_INLINE int32_t XTextureFile::getWidth(void) const
{ 
	return size_.x; 
}

X_INLINE int32_t XTextureFile::getHeight(void) const
{ 
	return size_.y; 
}

X_INLINE uint8_t XTextureFile::getNumFaces(void) const
{ 
	return numFaces_; 
}

X_INLINE uint8_t XTextureFile::getDepth(void) const
{ 
	return depth_; 
}

X_INLINE uint8_t XTextureFile::getNumMips(void) const
{ 
	return numMips_; 
}

X_INLINE size_t XTextureFile::getDataSize(void) const
{ 
	return data_.size(); 
}

X_INLINE TextureFlags XTextureFile::getFlags(void) const
{
	return flags_; 
}

X_INLINE Texturefmt::Enum XTextureFile::getFormat(void) const
{ 
	return format_; 
}

X_INLINE TextureType::Enum XTextureFile::getType(void) const
{
	return type_; 
}


X_INLINE const uint8_t* XTextureFile::getFace(size_t face) const
{
	return data_.ptr() + faceOffsets_[face];
}

X_INLINE uint8_t* XTextureFile::getFace(size_t face)
{
	return data_.ptr() + faceOffsets_[face];
}

X_INLINE const uint8_t* XTextureFile::getLevel(size_t face, size_t mip) const
{
	return data_.ptr() + (faceOffsets_[face] + mipOffsets_[mip]);
}

X_INLINE uint8_t* XTextureFile::getLevel(size_t face, size_t mip)
{
	return data_.ptr() + (faceOffsets_[face] + mipOffsets_[mip]);
}

X_INLINE size_t XTextureFile::getFaceSize(void) const
{
	if (depth_ > 1) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return data_.size();
}


X_INLINE void XTextureFile::setSize(const Vec2<uint16_t> size)
{ 
	this->size_ = size; 
}

X_INLINE void XTextureFile::setWidth(const uint16_t width)
{ 
	size_.x = width; 
}

X_INLINE void XTextureFile::setHeigth(const uint16_t height)
{
	size_.y = height; 
}

X_INLINE void XTextureFile::setFlags(TextureFlags	flags)
{ 
	this->flags_ = flags; 
}

X_INLINE void XTextureFile::setType(TextureType::Enum type)
{ 
	this->type_ = type; 
}

X_INLINE void XTextureFile::setFormat(Texturefmt::Enum format)
{ 
	this->format_ = format; 
}

X_INLINE void XTextureFile::setNumFaces(const int32_t faces)
{
	X_ASSERT(depth_ < std::numeric_limits<uint8_t>::max(), "invalid face count")(faces);
	numFaces_ = safe_static_cast<uint8_t, int32_t>(faces);
}

X_INLINE void XTextureFile::setDepth(const int32_t depth)
{
	X_ASSERT(depth < std::numeric_limits<uint8_t>::max(), "invalid depth")(depth);
	depth_ = safe_static_cast<uint8_t, int32_t>(depth);
}

X_INLINE void XTextureFile::setNumMips(const int32_t numMips)
{
	X_ASSERT(depth_ < std::numeric_limits<uint8_t>::max(), "invalid mipcount")(numMips);
	numMips_ = safe_static_cast<uint8_t, int32_t>(numMips);
}



X_NAMESPACE_END