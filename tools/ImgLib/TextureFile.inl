

X_NAMESPACE_BEGIN(texture)


X_INLINE const Vec2<uint16_t>& XTextureFile::getSize(void) const
{ 
	return size_; 
}
X_INLINE const int XTextureFile::getWidth(void) const
{ 
	return size_.x; 
}
X_INLINE const int XTextureFile::getHeight(void) const
{ 
	return size_.y; 
}
X_INLINE const uint8_t XTextureFile::getNumFaces(void) const
{ 
	return numFaces_; 
}
X_INLINE const uint8_t XTextureFile::getDepth(void) const
{ 
	return depth_; 
}
X_INLINE const uint8_t XTextureFile::getNumMips(void) const
{ 
	return numMips_; 
}
X_INLINE const uint32_t XTextureFile::getDataSize(void) const
{ 
	return datasize_; 
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


X_NAMESPACE_END