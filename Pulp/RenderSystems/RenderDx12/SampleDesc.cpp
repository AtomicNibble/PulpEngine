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
	SamplerDesc desc;
	samplerDescFromState(state, desc);

	return createDescriptor(allocator, desc);
}

SamplerDescriptor SamplerDescriptorCache::createDescriptor(DescriptorAllocator& allocator, const D3D12_SAMPLER_DESC& desc)
{
	const auto hash = hashDesc(desc);

	auto it = map_.find(hash);
	if (it != map_.end()) {
		return it->second;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptorHandle = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	pDevice_->CreateSampler(&desc, hCpuDescriptorHandle);

	SamplerDescriptor sampler(hCpuDescriptorHandle);

	map_.insert({hash, sampler});

	return sampler;
}

core::Hash::xxHash64::HashVal SamplerDescriptorCache::hashDesc(const D3D12_SAMPLER_DESC& desc)
{
	core::Hash::xxHash64 hasher;

	hasher.reset(0);
	hasher.update(desc);
	return hasher.finalize();
}

X_NAMESPACE_END

