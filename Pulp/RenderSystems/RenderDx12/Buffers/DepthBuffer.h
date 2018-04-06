#pragma once

#include "PixelBuffer.h"

X_NAMESPACE_BEGIN(render)

class DescriptorAllocator;

class DepthBuffer : public PixelBuffer
{
public:
    DepthBuffer(::texture::Texture& textInst, float32_t clearDepth = 1.0f, uint32_t clearStencil = 0);

    // Create a depth buffer. If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers.
    void create(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, DXGI_FORMAT format,
        D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

    void create(ID3D12Device* pDevice, DescriptorAllocator& allocator, const uint32_t width, uint32_t height, uint32_t numSamples,
        DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

    // Get pre-created CPU-visible descriptor handles
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getDSV(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getDSV_DepthReadOnly(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getDSV_StencilReadOnly(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getDSV_ReadOnly(void) const;
    const D3D12_CPU_DESCRIPTOR_HANDLE& getDepthSRV(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getStencilSRV(void) const;

    X_INLINE float32_t getClearDepth(void) const;
    X_INLINE uint32_t getClearStencil(void) const;

private:
    void createDerivedViews(ID3D12Device* pDevice, DescriptorAllocator& allocator, DXGI_FORMAT format);

private:
    float32_t clearDepth_;
    uint32_t clearStencil_;
    D3D12_CPU_DESCRIPTOR_HANDLE hDSV_[4];
    //  The shader resource view for depth, is now just stored in the texture::Texture(), as it makes drawing with it simple.
    //	But currently you can't ever bind the stencilSRV, not sure how to expose that in API.
    //	D3D12_CPU_DESCRIPTOR_HANDLE hDepthSRV_;
    D3D12_CPU_DESCRIPTOR_HANDLE hStencilSRV_;
};

X_NAMESPACE_END

#include "DepthBuffer.inl"