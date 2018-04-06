#pragma once

X_NAMESPACE_DECLARE(texture, class Texture);

X_NAMESPACE_BEGIN(render)

class GpuResource;

class PixelBuffer
{
public:
    X_DECLARE_FLAGS(ResourceFlag)
    (
        ALLOW_RENDER_TARGET,
        ALLOW_DEPTH_STENCIL,
        ALLOW_UNORDERED_ACCESS,
        DENY_SHADER_RESOURCE,
        ALLOW_CROSS_ADAPTER,
        ALLOW_SIMULTANEOUS_ACCES);

    typedef Flags<ResourceFlag> ResourceFlags;

protected:
    PixelBuffer(::texture::Texture& textInst);

    // these can be public
public:
    X_INLINE ::texture::Texture& getTex(void);
    X_INLINE const ::texture::Texture& getTex(void) const;

    render::GpuResource& getGpuResource(void);

protected:
    D3D12_RESOURCE_DESC describeTex2D(uint32_t width, uint32_t height,
        uint32_t depthOrArraySize, uint32_t numMips, DXGI_FORMAT format, uint32_t flags);

    void associateWithResource(ID3D12Device* pDevice, ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES currentState);

    void createTextureResource(ID3D12Device* pDevice, const D3D12_RESOURCE_DESC& resourceDesc,
        D3D12_CLEAR_VALUE clearValue, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

protected:
    static DXGI_FORMAT getBaseFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT getUAVFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT getDSVFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT getDepthFormat(DXGI_FORMAT Format);
    static DXGI_FORMAT getStencilFormat(DXGI_FORMAT Format);

protected:
    ::texture::Texture& textInst_;
};

X_NAMESPACE_END

#include "PixelBuffer.inl"