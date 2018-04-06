#pragma once

#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(render)

class DescriptorAllocator;

class SamplerDesc : public D3D12_SAMPLER_DESC
{
public:
    X_INLINE SamplerDesc();

    X_INLINE void setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE addressMode);
    X_INLINE void setBorderColor(Color8u border);
    X_INLINE void setBorderColor(const Colorf& border);
};

class SamplerDescriptor
{
public:
    SamplerDescriptor() = default;
    X_INLINE SamplerDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor);

    X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE getCpuDescriptorHandle(void) const;

protected:
    D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptorHandle_;
};

class SamplerDescriptorCache
{
    struct equal_to
    {
        bool operator()(const SamplerState lhs, const SamplerState rhs) const
        {
            return lhs.filter == rhs.filter && lhs.repeat == rhs.repeat;
        }
    };

    typedef core::HashMap<SamplerState, SamplerDescriptor, core::hash<SamplerState>, equal_to> DescriptorMap;

public:
    SamplerDescriptorCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice);

    SamplerDescriptor createDescriptor(DescriptorAllocator& allocator, const SamplerState state);

private:
    ID3D12Device* pDevice_;
    DescriptorMap map_;
};

X_NAMESPACE_END

#include "SamplerDesc.inl"