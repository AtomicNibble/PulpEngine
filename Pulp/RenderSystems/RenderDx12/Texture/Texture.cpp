#include "stdafx.h"
#include "Texture.h"

#include "Allocators\DescriptorAllocator.h"

#include "Texture\TextureUtil.h"

// Img Lib
#include <../../tools/ImgLib/ImgLib.h>

X_NAMESPACE_BEGIN(texture)

Texture::Texture(core::string_view name) :
    name_(name.data(), name.length())
{
    hCpuDescriptorHandle_.ptr = render::D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    dimensions_ = Vec2<uint16_t>::zero();
    datasize_ = 0;
    type_ = TextureType::UNKNOWN;
    format_ = Texturefmt::UNKNOWN;
    numMips_ = 0;
    depth_ = 0;
    numFaces_ = 0;

    usage_ = render::BufUsage::IMMUTABLE;
    pixelBufType_ = render::PixelBufferType::NONE;
    pPixelBuffer_ = nullptr;
}

Texture::~Texture()
{
#if X_DEBUG
    X_ASSERT(!pPixelBuffer_, "Dangling pixel buffer instance")(render::PixelBufferType::ToString(pixelBufType_), pPixelBuffer_);
#endif // !X_DEBUG
}

void Texture::destroy(void)
{
    resource_.destroy();
    hCpuDescriptorHandle_.ptr = render::D3D12_GPU_VIRTUAL_ADDRESS_NULL;
}

void Texture::setProperties(const XTextureFile& imgFile)
{
    // ummm check shit like generating mip maps and limits?
    // or converting a format if it's not support :S ?
    format_ = imgFile.getFormat();
    type_ = imgFile.getType();
    dimensions_.x = imgFile.getWidth();
    dimensions_.y = imgFile.getHeight();
    depth_ = imgFile.getDepth();
    numFaces_ = imgFile.getNumFaces();
    numMips_ = imgFile.getNumMips();
}

DXGI_FORMAT Texture::getFormatDX(void) const
{
    return Util::DXGIFormatFromTexFmt(format_);
}

void Texture::setPixelBuffer(render::PixelBufferType::Enum type, render::PixelBuffer* pInst)
{
    X_ASSERT(pixelBufType_ == render::PixelBufferType::NONE || type == render::PixelBufferType::NONE,"Already has a pixel buffer")(pixelBufType_);

    pixelBufType_ = type;
    pPixelBuffer_ = pInst;
}

X_NAMESPACE_END