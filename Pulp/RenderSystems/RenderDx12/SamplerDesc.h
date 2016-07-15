#pragma once


X_NAMESPACE_BEGIN(render)

class DescriptorAllocator;

class SamplerDesc : public D3D12_SAMPLER_DESC
{
public:
	SamplerDesc()
	{
		Filter = D3D12_FILTER_ANISOTROPIC;
		AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		MipLODBias = 0.0f;
		MaxAnisotropy = 16;
		ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		BorderColor[0] = 1.0f;
		BorderColor[1] = 1.0f;
		BorderColor[2] = 1.0f;
		BorderColor[3] = 1.0f;
		MinLOD = 0.0f;
		MaxLOD = D3D12_FLOAT32_MAX;
	}

	void setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE addressMode)
	{
		AddressU = addressMode;
		AddressV = addressMode;
		AddressW = addressMode;
	}

	void setBorderColor(Color8u border)
	{
		setBorderColor(Colorf(border));
	}
	void setBorderColor(const Colorf& border)
	{
		BorderColor[0] = border.r;
		BorderColor[1] = border.g;
		BorderColor[2] = border.b;
		BorderColor[3] = border.a;
	}
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