#include "stdafx.h"
#include "SamplerDesc.h"

#include "Allocators\DescriptorAllocator.h"

#include "Util\StateHelpers.h"

X_NAMESPACE_BEGIN(render)

// ---------------------------------

SamplerDescriptorCache::SamplerDescriptorCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
    pDevice_(pDevice),
    map_(arena, 64)
{
}

SamplerDescriptor SamplerDescriptorCache::createDescriptor(DescriptorAllocator& allocator, const SamplerState state)
{
    auto it = map_.find(state);
    if (it != map_.end()) {
        return it->second;
    }

    SamplerDesc desc;
    samplerDescFromState(state, desc);

    D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptorHandle = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    pDevice_->CreateSampler(&desc, hCpuDescriptorHandle);

    SamplerDescriptor sampler(hCpuDescriptorHandle);

    map_.insert({state, sampler});

    return sampler;
}

X_NAMESPACE_END
