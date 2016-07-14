#pragma once

#include "PixelBuffer.h"

X_NAMESPACE_BEGIN(render)



class ColorBuffer : public PixelBuffer
{
public:
	ColorBuffer();

	// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE& getSRV(void) const;
	const D3D12_CPU_DESCRIPTOR_HANDLE& getRTV(void) const;
	const D3D12_CPU_DESCRIPTOR_HANDLE& getUAV(void) const;

	Colorf getClearColor(void) const;

protected:
	void createDerivedViews(ID3D12Device* pDevice, DXGI_FORMAT format, 
		uint32_t rraySize, uint32_t NumMips = 1);

protected:
	Colorf clearColor_;
	D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle_[12];
	uint32_t numMipMaps_; // number of texture sublevels
};





X_NAMESPACE_END

#include "ColorBuffer.inl"