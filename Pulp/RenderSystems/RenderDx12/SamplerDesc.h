#pragma once


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

	void create(ID3D12Device* pDevice, DescriptorAllocator& allocator, const D3D12_SAMPLER_DESC& Desc);

	X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE getCpuDescriptorHandle(void) const;

protected:

	D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptorHandle_;
};

X_NAMESPACE_END

#include "SamplerDesc.inl"