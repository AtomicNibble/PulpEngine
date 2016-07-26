#include "stdafx.h"
#include "SamplerDesc.h"

#include "Allocators\DescriptorAllocator.h"

X_NAMESPACE_BEGIN(render)

void SamplerDescriptor::create(ID3D12Device* pDevice, DescriptorAllocator& allocator, const D3D12_SAMPLER_DESC& desc)
{
	hCpuDescriptorHandle_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	pDevice->CreateSampler(&desc, hCpuDescriptorHandle_);
}


X_NAMESPACE_END

