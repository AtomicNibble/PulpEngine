#pragma once

#include "PixelBuffer.h"
#include <ITexture.h>

X_NAMESPACE_BEGIN(render)

class DescriptorAllocator;

class ColorBuffer : public PixelBuffer, public IRenderTarget
{
public:
	ColorBuffer(Colorf clearCol = Colorf::zero());

	void createFromSwapChain(ID3D12Device* pDevice, DescriptorAllocator& allocator, ID3D12Resource* pBaseResource);

	void create(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, uint32_t numMips,
		DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
	void createArray(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, uint32_t arrayCount,
		DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);


	// Get pre-created CPU-visible descriptor handles
	X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getSRV(void) const;
	X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getRTV(void) const;
	X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getUAV(void) const;

	X_INLINE void setClearColor(const Colorf& col);
	X_INLINE Colorf getClearColor(void) const;

	// IRenderTarget
	texture::Texturefmt::Enum getFmt(void) X_FINAL;

	// ~IRenderTarget

protected:
	void createDerivedViews(ID3D12Device* pDevice, DescriptorAllocator& allocator, DXGI_FORMAT format,
		uint32_t rraySize, uint32_t NumMips = 1);

protected:
	Colorf clearColor_;
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle_[texture::TEX_MAX_MIPS];
	uint32_t numMipMaps_; // number of texture sublevels
};





X_NAMESPACE_END

#include "ColorBuffer.inl"