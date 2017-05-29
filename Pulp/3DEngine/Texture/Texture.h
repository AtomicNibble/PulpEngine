#pragma once


X_NAMESPACE_DECLARE(texture,	
	struct ITexture;
)

X_NAMESPACE_BEGIN(engine)


class Texture
{
public:
	Texture(core::string name, texture::TextureFlags flags);
	~Texture() = default;

	X_INLINE const int32_t getDeviceID(void) const;
	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	X_INLINE const core::string& getName(void) const;
	X_INLINE const Vec2<uint16_t> getDimensions(void) const;
	X_INLINE const int32_t getWidth(void) const;
	X_INLINE const int32_t getHeight(void) const;
	X_INLINE const int32_t getNumFaces(void) const;
	X_INLINE const int32_t getDepth(void) const;
	X_INLINE const int32_t getNumMips(void) const;
	X_INLINE const int32_t getDataSize(void) const;

	X_INLINE const bool isLoaded(void) const;
	X_INLINE const bool isLoading(void) const;
	X_INLINE const bool loadFailed(void) const;

	X_INLINE const texture::TextureType::Enum getTextureType(void) const;
	X_INLINE const texture::Texturefmt::Enum getFormat(void) const;
	X_INLINE const texture::TextureFlags getFlags(void) const;

	X_INLINE texture::TextureFlags& flags(void);

public:
	X_INLINE void setDeviceTexture(texture::ITexture* pTex);
	X_INLINE void setProperties(const texture::XTextureFile& imgFile);


private:
	core::string				fileName_;
	int32_t						id_;
	Vec2<uint16_t>				dimensions_;
	uint32_t					datasize_; // size of the higest mip in bytes.
	texture::TextureType::Enum	type_;
	texture::TextureFlags		flags_;
	texture::Texturefmt::Enum 	format_;
	uint8_t						numMips_;
	uint8_t						depth_;
	uint8_t						numFaces_;

	texture::ITexture*			pDeviceTexture_;
};


X_NAMESPACE_END

#include "Texture.inl"