#pragma once

#include "GpuResource.h"

X_NAMESPACE_BEGIN(render)


class PixelBuffer : public GpuResource
{
public:
	X_DECLARE_FLAGS(ResourceFlag) (
		ALLOW_RENDER_TARGET,
		ALLOW_DEPTH_STENCIL,
		ALLOW_UNORDERED_ACCESS,
		DENY_SHADER_RESOURCE,
		ALLOW_CROSS_ADAPTER,
		ALLOW_SIMULTANEOUS_ACCES
	);

	typedef Flags<ResourceFlag> ResourceFlags;

public:
	PixelBuffer();
	
	X_INLINE uint32_t getWidth(void) const;
	X_INLINE uint32_t getHeight(void) const;
	X_INLINE uint32_t getDepth(void) const;
	X_INLINE const DXGI_FORMAT getFormat(void) const;

protected:

	D3D12_RESOURCE_DESC describeTex2D(uint32_t width, uint32_t height,
		uint32_t depthOrArraySize, uint32_t numMips, DXGI_FORMAT format, uint32_t flags);

	void associateWithResource(ID3D12Device* pDevice, ID3D12Resource* pResource, 
		D3D12_RESOURCE_STATES currentState);

	void createTextureResource(ID3D12Device* pDevice, const D3D12_RESOURCE_DESC& resourceDesc,
		D3D12_CLEAR_VALUE clearValue, D3D12_GPU_VIRTUAL_ADDRESS 
		vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);


	static DXGI_FORMAT getBaseFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT getUAVFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT getDSVFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT getDepthFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT getStencilFormat(DXGI_FORMAT Format);

	uint32_t width_;
	uint32_t height_;
	uint32_t arraySize_; // depth
	DXGI_FORMAT format_;
};



X_NAMESPACE_END

#include "PixelBuffer.inl"