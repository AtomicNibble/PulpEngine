#include "stdafx.h"
#include "Texture.h"

#include "Allocators\DescriptorAllocator.h"

#include "Texture\TextureUtil.h"

X_NAMESPACE_BEGIN(texture)


	Texture::Texture(const char* pName, TextureFlags flags) :
		fileName_(pName),
		flags_(flags)
	{
		hCpuDescriptorHandle_.ptr = render::D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		dimensions_ = Vec2<uint16_t>::zero();
		datasize_ = 0;
		type_ = TextureType::UNKNOWN;
		format_ = Texturefmt::UNKNOWN;
		numMips_ = 0;
		depth_ = 0;
		numFaces_ = 0;

		pixelBufType_ = render::PixelBufferType::NONE;
		pPixelBuffer_ = nullptr;
	}

	Texture::Texture(core::string name, TextureFlags flags) :
		fileName_(name),
		flags_(flags)
	{
		hCpuDescriptorHandle_.ptr = render::D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		dimensions_ = Vec2<uint16_t>::zero();
		datasize_ = 0;
		type_ = TextureType::UNKNOWN;
		format_ = Texturefmt::UNKNOWN;
		numMips_ = 0;
		depth_ = 0;
		numFaces_ = 0;

		pixelBufType_ = render::PixelBufferType::NONE;
		pPixelBuffer_ = nullptr;
	}




	Texture::~Texture()
	{

	}

	void Texture::destroy(void)
	{
		resource_.destroy();
		hCpuDescriptorHandle_.ptr = 0;
	}


	const DXGI_FORMAT Texture::getFormatDX(void) const
	{
		return Util::DXGIFormatFromTexFmt(format_);
	}

	void Texture::setPixelBuffer(render::PixelBufferType::Enum type, render::PixelBuffer* pInst)
	{
		X_ASSERT(pixelBufType_ == render::PixelBufferType::NONE, "Already has a pixel buffer")(pixelBufType_);

		pixelBufType_ = type;
		pPixelBuffer_ = pInst;
	}


X_NAMESPACE_END