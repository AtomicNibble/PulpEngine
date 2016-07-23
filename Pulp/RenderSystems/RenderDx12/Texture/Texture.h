#pragma once

#include <ITexture.h>

X_NAMESPACE_BEGIN(texture)


	class X_ALIGNED_SYMBOL(Texture, 64) : public ::texture::ITexture
	{

	public:
		Texture(const char* pName, TextureFlags flags);
		~Texture();

		// temp maybe
		X_INLINE const int addRef() X_OVERRIDE { return 1; }
		X_INLINE const int release() X_OVERRIDE { return 0; }
		X_INLINE const int forceRelease() X_OVERRIDE { return 0;}
		X_INLINE const TexID getTexID(void) const X_OVERRIDE { return 0; };
		X_INLINE const bool IsShared() const X_OVERRIDE { return false; }


		X_INLINE const char* getName(void) const X_OVERRIDE;
		X_INLINE const Vec2<uint16_t> getDimensions(void) const X_OVERRIDE;
		X_INLINE const int32_t getWidth(void) const X_OVERRIDE;
		X_INLINE const int32_t getHeight(void) const X_OVERRIDE;
		X_INLINE const int32_t getNumFaces(void) const X_OVERRIDE;
		X_INLINE const int32_t getDepth(void) const X_OVERRIDE;
		X_INLINE const int32_t getNumMips(void) const X_OVERRIDE;
		X_INLINE const int32_t getDataSize(void) const X_OVERRIDE;

		X_INLINE const bool isLoaded(void) const X_OVERRIDE;
		X_INLINE const bool IsStreamable(void) const X_OVERRIDE;

		X_INLINE const TextureType::Enum getTextureType(void) const X_OVERRIDE;
		X_INLINE const TextureFlags getFlags(void) const X_OVERRIDE;
		X_INLINE const Texturefmt::Enum getFormat(void) const X_OVERRIDE;

	private:
		core::string		fileName_;
		Vec2<uint16_t>		dimensions_;
		uint32_t			datasize_; // size of the higest mip in bytes.
		TextureType::Enum	type_;
		TextureFlags		flags_;
		Texturefmt::Enum 	format_;
		uint8_t				numMips_;
		uint8_t				depth_;
		uint8_t				numFaces_;
	};



X_NAMESPACE_END

#include "Texture.inl"