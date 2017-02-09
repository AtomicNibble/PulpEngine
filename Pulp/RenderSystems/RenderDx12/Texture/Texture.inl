
X_NAMESPACE_BEGIN(texture)


	X_INLINE const core::string& Texture::getName(void) const
	{
		return fileName_;
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
		return flags_.IsSet(TextureFlags::STREAMABLE);
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

	// IPixelBuffer
	X_INLINE render::PixelBufferType::Enum Texture::getBufferType(void) const
	{
		return render::PixelBufferType::NONE;
	}

	// ~IPixelBuffer


	X_INLINE render::GpuResource& Texture::getGpuResource(void)
	{
		return resource_;
	}

	X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& Texture::getSRV(void) const
	{
		return hCpuDescriptorHandle_;
	}


	X_INLINE const int32_t Texture::getID(void) const
	{
		return id_;
	}

	X_INLINE void Texture::setID(TexID id)
	{
		id_ = id;
	}

	X_INLINE void Texture::setSRV(D3D12_CPU_DESCRIPTOR_HANDLE& srv)
	{
		hCpuDescriptorHandle_ = srv;
	}



	X_INLINE void Texture::setFormat(Texturefmt::Enum fmt)
	{
		format_ = fmt;
	}

	X_INLINE void Texture::setType(TextureType::Enum type)
	{
		type_ = type;
	}

	X_INLINE void Texture::setWidth(uint16_t width)
	{
		dimensions_.x = width;
	}

	X_INLINE void Texture::setHeight(uint16_t height)
	{
		dimensions_.y = height;
	}

	X_INLINE void Texture::setDepth(uint8_t depth)
	{
		depth_ = depth;
	}

	X_INLINE void Texture::setNumFaces(uint8_t faces)
	{
		numFaces_ = faces;
	}

	X_INLINE void Texture::setNumMips(uint8_t mips)
	{
		numMips_ = mips;
	}


X_NAMESPACE_END