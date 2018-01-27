
X_NAMESPACE_BEGIN(render)


X_INLINE SamplerDesc::SamplerDesc()
{
	Filter = D3D12_FILTER_ANISOTROPIC;
	AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	MipLODBias = 0.0f;
	MaxAnisotropy = 16;
	ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	BorderColor[0] = 1.0f;
	BorderColor[1] = 1.0f;
	BorderColor[2] = 1.0f;
	BorderColor[3] = 1.0f;
	MinLOD = 0.0f;
	MaxLOD = D3D12_FLOAT32_MAX;
}

X_INLINE void SamplerDesc::setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE addressMode)
{
	AddressU = addressMode;
	AddressV = addressMode;
	AddressW = addressMode;
}

X_INLINE void SamplerDesc::setBorderColor(Color8u border)
{
	setBorderColor(Colorf(border));
}

X_INLINE void SamplerDesc::setBorderColor(const Colorf& border)
{
	BorderColor[0] = border.r;
	BorderColor[1] = border.g;
	BorderColor[2] = border.b;
	BorderColor[3] = border.a;
}

// ---------------------------------------------------

X_INLINE SamplerDescriptor::SamplerDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor)
	: hCpuDescriptorHandle_(hCpuDescriptor)
{

}

X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE SamplerDescriptor::getCpuDescriptorHandle(void) const
{
	return hCpuDescriptorHandle_;
}


X_NAMESPACE_END