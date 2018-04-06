#include "stdafx.h"
#include "Texture.h"

X_NAMESPACE_BEGIN(engine)

Texture::Texture(core::string name, texture::TextureFlags flags, render::IDeviceTexture* pDeviceTexture) :
    core::AssetBase(name, assetDb::AssetType::IMG),
    flags_(flags),
    pDeviceTexture_(X_ASSERT_NOT_NULL(pDeviceTexture))
{
    dimensions_ = Vec2<uint16_t>::zero();
    datasize_ = 0;
    type_ = texture::TextureType::UNKNOWN;
    format_ = texture::Texturefmt::UNKNOWN;
    numMips_ = 0;
    depth_ = 0;
    numFaces_ = 0;
}

X_NAMESPACE_END