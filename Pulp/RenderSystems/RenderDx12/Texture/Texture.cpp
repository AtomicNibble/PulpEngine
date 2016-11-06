#include "stdafx.h"
#include "Texture.h"

#include "Allocators\DescriptorAllocator.h"

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
	}




	Texture::~Texture()
	{

	}

	void Texture::destroy(void)
	{
		resource_.destroy();
		hCpuDescriptorHandle_.ptr = 0;
	}


X_NAMESPACE_END