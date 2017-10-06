
X_NAMESPACE_BEGIN(engine)



X_INLINE const int32_t Texture::getDeviceID(void) const
{
	X_ASSERT_NOT_NULL(pDeviceTexture_);
	return pDeviceTexture_->getTexID();
}

X_INLINE const Vec2<uint16_t> Texture::getDimensions(void) const
{
	return dimensions_;
}

X_INLINE const int32_t Texture::getWidth(void) const
{
	return dimensions_.x;
}

X_INLINE const int32_t Texture::getHeight(void) const
{
	return dimensions_.y;
}

X_INLINE const int32_t Texture::getNumFaces(void) const
{
	return numFaces_;
}

X_INLINE const int32_t Texture::getDepth(void) const
{
	return depth_;
}

X_INLINE const int32_t Texture::getNumMips(void) const
{
	return static_cast<int32_t>(numMips_);
}

X_INLINE const int32_t Texture::getDataSize(void) const
{
	return 0;
}

X_INLINE const texture::TextureType::Enum Texture::getTextureType(void) const
{
	return type_;
}

X_INLINE const texture::Texturefmt::Enum Texture::getFormat(void) const
{
	return format_;
}

X_INLINE const texture::TextureFlags Texture::getFlags(void) const
{
	return flags_;
}


X_INLINE texture::TextureFlags& Texture::flags(void)
{
	return flags_;
}


// --------------------------------------


X_INLINE void Texture::setProperties(const texture::XTextureFile& imgFile)
{
	dimensions_ = imgFile.getSize();
	datasize_ = static_cast<uint32_t>(imgFile.getDataSize());
	type_ = imgFile.getType();
	flags_ = imgFile.getFlags();
	format_ = imgFile.getFormat();
	numMips_ = static_cast<uint8_t>(imgFile.getNumMips());
	numFaces_ = static_cast<uint8_t>(imgFile.getNumFaces());
	depth_ = static_cast<uint8_t>(imgFile.getDepth());

	pDeviceTexture_->setProperties(imgFile);
}

X_INLINE render::IDeviceTexture* Texture::getDeviceTexture(void) const
{
	return pDeviceTexture_;
}

X_NAMESPACE_END