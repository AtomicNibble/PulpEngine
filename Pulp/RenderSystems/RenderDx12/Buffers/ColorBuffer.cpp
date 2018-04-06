#include "stdafx.h"
#include "ColorBuffer.h"
#include "Allocators\DescriptorAllocator.h"

#include "Texture\Texture.h"
#include "Texture\TextureUtil.h"

X_NAMESPACE_BEGIN(render)

ColorBuffer::ColorBuffer(::texture::Texture& textInst, Colorf clearCol) :
    clearColor_(clearCol),
    PixelBuffer(textInst)
{
    // SRVHandle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    RTVHandle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

void ColorBuffer::createDerivedViews(ID3D12Device* pDevice, DescriptorAllocator& allocator,
    DXGI_FORMAT format, uint32_t arraySize, uint32_t numMips)
{
    X_ASSERT(arraySize == 1 || numMips == 1, "auto-mips on texture arrays not supported")
    (arraySize, numMips);
    X_ASSERT(numMips < texture::TEX_MAX_MIPS, "numMips exceeds max")
    (numMips, texture::TEX_MAX_MIPS);

    auto& tex = getTex();
    tex.setNumMips(numMips - 1);

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;

    RTVDesc.Format = format;
    UAVDesc.Format = getUAVFormat(format);
    SRVDesc.Format = format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if (arraySize > 1) {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        RTVDesc.Texture2DArray.MipSlice = 0;
        RTVDesc.Texture2DArray.FirstArraySlice = 0;
        RTVDesc.Texture2DArray.ArraySize = arraySize;

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UAVDesc.Texture2DArray.MipSlice = 0;
        UAVDesc.Texture2DArray.FirstArraySlice = 0;
        UAVDesc.Texture2DArray.ArraySize = arraySize;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        SRVDesc.Texture2DArray.MipLevels = numMips;
        SRVDesc.Texture2DArray.MostDetailedMip = 0;
        SRVDesc.Texture2DArray.FirstArraySlice = 0;
        SRVDesc.Texture2DArray.ArraySize = arraySize;
    }
    else {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        RTVDesc.Texture2D.MipSlice = 0;

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        UAVDesc.Texture2D.MipSlice = 0;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = numMips;
        SRVDesc.Texture2D.MostDetailedMip = 0;
    }

    auto& resource = tex.getGpuResource();

    if (tex.getSRV().ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        RTVHandle_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        tex.setSRV(allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    }

    // Create the render target view
    pDevice->CreateRenderTargetView(resource.getResource(), &RTVDesc, RTVHandle_);

    // Create the shader resource view
    pDevice->CreateShaderResourceView(resource.getResource(), &SRVDesc, tex.getSRV());

    UAVHandles_.resize(numMips);
    std::fill(UAVHandles_.begin(), UAVHandles_.end(), CD3DX12_CPU_DESCRIPTOR_HANDLE());

    X_ASSERT(UAVHandles_[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, "Incorrect initial value")
    ();

    // Create the UAVs for each mip level (RWTexture2D)
    D3D12_CPU_DESCRIPTOR_HANDLE* pUAVHandles = getUAVs();

    for (uint32_t i = 0; i < numMips; ++i) {
        D3D12_CPU_DESCRIPTOR_HANDLE& pMipUAV = pUAVHandles[i];

        if (pMipUAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
            pMipUAV = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        pDevice->CreateUnorderedAccessView(resource.getResource(), nullptr, &UAVDesc, pMipUAV);

        UAVDesc.Texture2D.MipSlice++;
    }
}

void ColorBuffer::createFromSwapChain(ID3D12Device* pDevice, DescriptorAllocator& allocator, ID3D12Resource* pBaseResource)
{
    associateWithResource(pDevice, pBaseResource, D3D12_RESOURCE_STATE_PRESENT);

    auto& tex = getTex();
    auto& resource = tex.getGpuResource();

    RTVHandle_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    pDevice->CreateRenderTargetView(resource.getResource(), nullptr, RTVHandle_);
}

void ColorBuffer::create(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, uint32_t numMips,
    DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    D3D12_RESOURCE_DESC resourceDesc = describeTex2D(width, height, 1, numMips, format,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_CLEAR_VALUE ClearValue;
    core::zero_object(ClearValue);
    ClearValue.Format = format;
    ClearValue.Color[0] = clearColor_[0];
    ClearValue.Color[1] = clearColor_[1];
    ClearValue.Color[2] = clearColor_[2];
    ClearValue.Color[3] = clearColor_[3];

    createTextureResource(pDevice, resourceDesc, ClearValue, vidMemPtr);
    createDerivedViews(pDevice, allocator, format, 1, numMips);
}

void ColorBuffer::createArray(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height, uint32_t arrayCount,
    DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    D3D12_RESOURCE_DESC resourceDesc = describeTex2D(width, height, arrayCount, 1, format,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_CLEAR_VALUE ClearValue;
    core::zero_object(ClearValue);
    ClearValue.Format = format;
    ClearValue.Color[0] = clearColor_[0];
    ClearValue.Color[1] = clearColor_[1];
    ClearValue.Color[2] = clearColor_[2];
    ClearValue.Color[3] = clearColor_[3];

    createTextureResource(pDevice, resourceDesc, ClearValue, vidMemPtr);
    createDerivedViews(pDevice, allocator, format, arrayCount, 1);
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& ColorBuffer::getSRV(void) const
{
    return getTex().getSRV();
}

X_NAMESPACE_END