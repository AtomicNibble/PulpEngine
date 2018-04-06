#include "stdafx.h"
#include "ShadowBuffer.h"
#include "CommandContex.h"

#include "Texture\Texture.h"

X_NAMESPACE_BEGIN(render)

ShadowBuffer::ShadowBuffer(::texture::Texture& textInst) :
    DepthBuffer(textInst)
{
}

void ShadowBuffer::create(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t width, uint32_t height,
    D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr)
{
    DepthBuffer::create(pDevice, allocator, width, height, DXGI_FORMAT_D16_UNORM, vidMemPtr);

    viewport_.TopLeftX = 0.0f;
    viewport_.TopLeftY = 0.0f;
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    // Prevent drawing to the boundary pixels so that we don't have to worry about shadows stretching
    scissor_.left = 1;
    scissor_.top = 1;
    scissor_.right = static_cast<LONG>(width - 2);
    scissor_.bottom = static_cast<LONG>(height - 2);
}

D3D12_CPU_DESCRIPTOR_HANDLE ShadowBuffer::getSRV(void) const
{
    return getDepthSRV();
}

void ShadowBuffer::beginRendering(GraphicsContext& context)
{
    auto& resource = getGpuResource();

    context.transitionResource(resource, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
    context.clearDepth(*this);
    context.setDepthStencilTarget(getDSV());
    context.setViewportAndScissor(viewport_, scissor_);
}

void ShadowBuffer::endRendering(GraphicsContext& context)
{
    auto& resource = getGpuResource();

    context.transitionResource(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

X_NAMESPACE_END