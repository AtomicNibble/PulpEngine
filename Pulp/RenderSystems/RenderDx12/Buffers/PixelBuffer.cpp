#include "stdafx.h"
#include "PixelBuffer.h"

#include "Texture\Texture.h"
#include "Texture\TextureUtil.h"

X_NAMESPACE_BEGIN(render)

PixelBuffer::PixelBuffer(texture::Texture& textInst) :
    textInst_(textInst)
{
}

render::GpuResource& PixelBuffer::getGpuResource(void)
{
    return textInst_.getGpuResource();
}

D3D12_RESOURCE_DESC PixelBuffer::describeTex2D(uint32_t width, uint32_t height,
    uint32_t depthOrArraySize, uint32_t numMips, DXGI_FORMAT format, uint32_t flags)
{
    auto& tex = getTex();

    tex.setWidth(safe_static_cast<uint16_t>(width));
    tex.setHeight(safe_static_cast<uint16_t>(height));
    tex.setNumMips(safe_static_cast<uint8_t>(depthOrArraySize));
    tex.setFormat(texture::Util::texFmtFromDXGI(format));

    D3D12_RESOURCE_DESC desc;
    core::zero_object(desc);
    desc.Alignment = 0;
    desc.DepthOrArraySize = safe_static_cast<uint16_t, uint32_t>(depthOrArraySize);
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Flags = (D3D12_RESOURCE_FLAGS)flags;
    desc.Format = getBaseFormat(format);
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.MipLevels = numMips;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Height = static_cast<uint32_t>(height);
    desc.Width = static_cast<uint64_t>(width);
    return desc;
}

void PixelBuffer::associateWithResource(ID3D12Device* pDevice, ID3D12Resource* pResource,
    D3D12_RESOURCE_STATES currentState)
{
    X_ASSERT_NOT_NULL(pResource);
    D3D12_RESOURCE_DESC resourceDesc = pResource->GetDesc();

    auto& tex = getTex();
    auto& resource = tex.getGpuResource();

    resource.getResourcePtrRef() = pResource;
    resource.setUsageState(currentState);

    tex.setWidth(safe_static_cast<uint16_t>(resourceDesc.Width));
    tex.setHeight(safe_static_cast<uint16_t>(resourceDesc.Height));
    tex.setNumMips(safe_static_cast<uint8_t>(resourceDesc.DepthOrArraySize));
    tex.setFormat(texture::Util::texFmtFromDXGI(resourceDesc.Format));
}

void PixelBuffer::createTextureResource(ID3D12Device* pDevice,
    const D3D12_RESOURCE_DESC& resourceDesc, D3D12_CLEAR_VALUE clearValue,
    D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    auto& resource = getTex().getGpuResource();

    HRESULT hr = pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource.getResourcePtrRef()));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create commited resource. err: %" PRIu32, hr);
    }

    resource.setUsageState(D3D12_RESOURCE_STATE_COMMON);
    resource.setGpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL);
}

DXGI_FORMAT PixelBuffer::getBaseFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;

        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8X8_TYPELESS;

            // 32-bit Z w/ Stencil
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return DXGI_FORMAT_R32G8X24_TYPELESS;

            // No Stencil
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
            return DXGI_FORMAT_R32_TYPELESS;

            // 24-bit Z
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;

            // 16-bit Z w/o Stencil
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
            return DXGI_FORMAT_R16_TYPELESS;

        default:
            return defaultFormat;
    }
}

DXGI_FORMAT PixelBuffer::getUAVFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;

        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_UNORM;

        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8X8_UNORM;

        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;

#if X_DEBUG
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_D16_UNORM:

            // Requested a UAV format for a depth stencil format.
            X_ASSERT_UNREACHABLE();

#endif // X_DEBUG

        default:
            return defaultFormat;
    }
}

DXGI_FORMAT PixelBuffer::getDSVFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat) {
            // 32-bit Z w/ Stencil
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

            // No Stencil
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;

            // 24-bit Z
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;

            // 16-bit Z w/o Stencil
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
            return DXGI_FORMAT_D16_UNORM;

        default:
            return defaultFormat;
    }
}

DXGI_FORMAT PixelBuffer::getDepthFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat) {
            // 32-bit Z w/ Stencil
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

            // No Stencil
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;

            // 24-bit Z
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

            // 16-bit Z w/o Stencil
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT PixelBuffer::getStencilFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat) {
            // 32-bit Z w/ Stencil
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

            // 24-bit Z
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

X_NAMESPACE_END