#include "stdafx.h"
#include "SamplerDesc.h"

#include "DescriptorAllocator.h"

X_NAMESPACE_BEGIN(render)

void SamplerDescriptor::create(ID3D12Device* pDevice, DescriptorAllocator* pAllocator, const D3D12_SAMPLER_DESC& desc)
{
	hCpuDescriptorHandle_ = pAllocator->allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	pDevice->CreateSampler(&desc, hCpuDescriptorHandle_);
}


X_NAMESPACE_END

