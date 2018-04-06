#pragma once

X_NAMESPACE_BEGIN(render)

X_DECLARE_ENUM(DescriptorHeapType)
(
    CBV_SRV_UAV,
    SAMPLER);

static_assert(DescriptorHeapType::CBV_SRV_UAV == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Value not match");
static_assert(DescriptorHeapType::SAMPLER == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Value not match");

X_NAMESPACE_END