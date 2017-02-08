#include "stdafx.h"
#include "DepthBuffer.h"
#include "Allocators\DescriptorAllocator.h"


X_NAMESPACE_BEGIN(render)

DepthBuffer::DepthBuffer(float32_t clearDepth, uint32_t clearStencil) :
	clearDepth_(clearDepth),
	clearStencil_(clearStencil)
{
	hDSV_[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	hDSV_[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	hDSV_[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	hDSV_[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	hDepthSRV_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	hStencilSRV_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

// Create a depth buffer. If an address is supplied, memory will not be allocated.
// The vmem address allows you to alias buffers.
void DepthBuffer::create(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, DXGI_FORMAT format,
	D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
	D3D12_RESOURCE_DESC resourceDesc = describeTex2D(width, height, 1, 1, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE ClearValue;
	core::zero_object(ClearValue);
	ClearValue.Format = format;

	createTextureResource(pDevice, resourceDesc, ClearValue, vidMemPtr);
	createDerivedViews(pDevice, allocator, format);
}

void DepthBuffer::create(ID3D12Device* pDevice, DescriptorAllocator& allocator, const uint32_t width, uint32_t height, uint32_t numSamples, 
	DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
	D3D12_RESOURCE_DESC resourceDesc = describeTex2D(width, height, 1, 1, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	resourceDesc.SampleDesc.Count = numSamples;

	D3D12_CLEAR_VALUE ClearValue;
	core::zero_object(ClearValue);
	ClearValue.Format = format;

	createTextureResource(pDevice, resourceDesc, ClearValue, vidMemPtr);
	createDerivedViews(pDevice, allocator, format);
}


void DepthBuffer::createDerivedViews(ID3D12Device* pDevice, DescriptorAllocator& allocator, DXGI_FORMAT format)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = getDSVFormat(format);
	if (pResource_->GetDesc().SampleDesc.Count == 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	if (hDSV_[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		hDSV_[0] = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		hDSV_[1] = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	pDevice->CreateDepthStencilView(pResource_, &dsvDesc, hDSV_[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	pDevice->CreateDepthStencilView(pResource_, &dsvDesc, hDSV_[1]);

	DXGI_FORMAT stencilReadFormat = getStencilFormat(format);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (hDSV_[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			hDSV_[2] = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			hDSV_[3] = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		pDevice->CreateDepthStencilView(pResource_, &dsvDesc, hDSV_[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		pDevice->CreateDepthStencilView(pResource_, &dsvDesc, hDSV_[3]);
	}
	else
	{
		hDSV_[2] = hDSV_[0];
		hDSV_[3] = hDSV_[1];
	}

	if (hDepthSRV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		hDepthSRV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = getDepthFormat(format);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	pDevice->CreateShaderResourceView(pResource_, &SRVDesc, hDepthSRV_);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (hStencilSRV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			hStencilSRV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		SRVDesc.Format = stencilReadFormat;
		pDevice->CreateShaderResourceView(pResource_, &SRVDesc, hStencilSRV_);
	}
}


X_NAMESPACE_END