
X_NAMESPACE_BEGIN(texture)


	X_INLINE const char* Texture::getName(void) const
	{
		return fileName_.c_str();
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


	X_INLINE const bool Texture::isLoaded(void) const
	{
		return !flags_.IsSet(TextureFlags::LOAD_FAILED);
	}

	X_INLINE const bool Texture::IsStreamable(void) const
	{
		return !flags_.IsSet(TextureFlags::DONT_STREAM);
	}

	X_INLINE const TextureType::Enum Texture::getTextureType(void) const
	{
		return type_;
	}

	X_INLINE const TextureFlags Texture::getFlags(void) const
	{
		return flags_;
	}

	X_INLINE const Texturefmt::Enum Texture::getFormat(void) const
	{
		return format_;
	}

X_NAMESPACE_END