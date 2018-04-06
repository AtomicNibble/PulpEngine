#pragma once

#include "DepthBuffer.h"

X_NAMESPACE_BEGIN(render)

class GraphicsContext;

class ShadowBuffer : public DepthBuffer
{
public:
    ShadowBuffer(::texture::Texture& textInst);

    void create(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height,
        D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

    D3D12_CPU_DESCRIPTOR_HANDLE getSRV(void) const;

    void beginRendering(GraphicsContext& context);
    void endRendering(GraphicsContext& context);

private:
    D3D12_VIEWPORT viewport_;
    D3D12_RECT scissor_;
};

X_NAMESPACE_END