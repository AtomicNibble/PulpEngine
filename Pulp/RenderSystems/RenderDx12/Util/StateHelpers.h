#pragma once

X_NAMESPACE_BEGIN(render)

class SamplerDesc;

void createDescFromState(const StateDesc& state, D3D12_BLEND_DESC& blendDesc);
void createDescFromState(const StateDesc& state, D3D12_RASTERIZER_DESC& rasterizerDesc);
void createDescFromState(const StateDesc& stateDesc, D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);

X_INLINE D3D12_PRIMITIVE_TOPOLOGY_TYPE topoTypeFromDesc(const StateDesc& desc);
X_INLINE D3D12_PRIMITIVE_TOPOLOGY topoFromDesc(const StateDesc& desc);
void samplerDescFromState(SamplerState state, SamplerDesc& desc);

X_NAMESPACE_END

#include "StateHelpers.inl"