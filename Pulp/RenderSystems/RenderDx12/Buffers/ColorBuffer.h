#pragma once

#include "PixelBuffer.h"
#include <ITexture.h>

#include <Util\UniquePointer.h>

X_NAMESPACE_BEGIN(render)

class DescriptorAllocator;

class ColorBuffer : public PixelBuffer
{
public:
    typedef core::FixedArray<D3D12_CPU_DESCRIPTOR_HANDLE, texture::TEX_MAX_MIPS> DescriptorHandleArr;

public:
    ColorBuffer(::texture::Texture& textInst, Colorf clearCol = Colorf::zero());

    void createFromSwapChain(ID3D12Device* pDevice, DescriptorAllocator& allocator, ID3D12Resource* pBaseResource);

    void create(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, uint32_t numMips,
        DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
    void createArray(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, uint32_t arrayCount,
        DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& getSRV(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getRTV(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getUAV(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE* getUAVs(void) const;

    X_INLINE void setClearColor(const Colorf& col);
    X_INLINE Colorf getClearColor(void) const;

protected:
    X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE* getUAVs(void);

protected:
    void createDerivedViews(ID3D12Device* pDevice, DescriptorAllocator& allocator, DXGI_FORMAT format,
        uint32_t rraySize, uint32_t NumMips = 1);

protected:
    Color8u clearColor_;
    D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle_;

    DescriptorHandleArr UAVHandles_;
};

X_NAMESPACE_END

#include "ColorBuffer.inl"