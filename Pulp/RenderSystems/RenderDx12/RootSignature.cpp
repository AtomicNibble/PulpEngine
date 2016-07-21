#include "stdafx.h"
#include "RootSignature.h"

X_NAMESPACE_BEGIN(render)


void RootSignature::clear(void)
{
	params_.clear();
	samplers_.clear();

	samplesInitCount_ = 0;

	core::SafeReleaseDX(pSignature_);
}

void RootSignature::free(void)
{
	clear();

	params_.free();
	samplers_.free();

}


void RootSignature::initStaticSampler(uint32_t Register, const D3D12_SAMPLER_DESC& nonStaticSamplerDesc,
	D3D12_SHADER_VISIBILITY visibility)
{
	D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc = samplers_[samplesInitCount_++];

	staticSamplerDesc.Filter = nonStaticSamplerDesc.Filter;
	staticSamplerDesc.AddressU = nonStaticSamplerDesc.AddressU;
	staticSamplerDesc.AddressV = nonStaticSamplerDesc.AddressV;
	staticSamplerDesc.AddressW = nonStaticSamplerDesc.AddressW;
	staticSamplerDesc.MipLODBias = nonStaticSamplerDesc.MipLODBias;
	staticSamplerDesc.MaxAnisotropy = nonStaticSamplerDesc.MaxAnisotropy;
	staticSamplerDesc.ComparisonFunc = nonStaticSamplerDesc.ComparisonFunc;
	staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplerDesc.MinLOD = nonStaticSamplerDesc.MinLOD;
	staticSamplerDesc.MaxLOD = nonStaticSamplerDesc.MaxLOD;
	staticSamplerDesc.ShaderRegister = Register;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = visibility;

	if (staticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
		staticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
		staticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER)
	{
		if (!(
			// Transparent Black
			nonStaticSamplerDesc.BorderColor[0] == 0.0f &&
			nonStaticSamplerDesc.BorderColor[1] == 0.0f &&
			nonStaticSamplerDesc.BorderColor[2] == 0.0f &&
			nonStaticSamplerDesc.BorderColor[3] == 0.0f ||
			// Opaque Black
			nonStaticSamplerDesc.BorderColor[0] == 0.0f &&
			nonStaticSamplerDesc.BorderColor[1] == 0.0f &&
			nonStaticSamplerDesc.BorderColor[2] == 0.0f &&
			nonStaticSamplerDesc.BorderColor[3] == 1.0f ||
			// Opaque White
			nonStaticSamplerDesc.BorderColor[0] == 1.0f &&
			nonStaticSamplerDesc.BorderColor[1] == 1.0f &&
			nonStaticSamplerDesc.BorderColor[2] == 1.0f &&
			nonStaticSamplerDesc.BorderColor[3] == 1.0f))
		{
			X_WARNING("Dx12", "Sampler border color does not match static sampler limitations");
		}		

		if (nonStaticSamplerDesc.BorderColor[3] == 1.0f)
		{
			if (nonStaticSamplerDesc.BorderColor[0] == 1.0f) {
				staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			}
			else {
				staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			}
		}
		else {
			staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		}
	}
}

void RootSignature::finalize(ID3D12Device* pDevice, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	if (pSignature_) {
		return;
	}
	
	X_ASSERT_NOT_NULL(pDevice);
	X_ASSERT(samplesInitCount_ == static_cast<uint32_t>(samplers_.size()), "Not all samplers are init")(samplesInitCount_, samplers_.size());

	D3D12_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.NumParameters = safe_static_cast<uint32_t, size_t>(params_.size());
	rootDesc.pParameters = reinterpret_cast<const D3D12_ROOT_PARAMETER*>(params_.data());
	rootDesc.NumStaticSamplers = safe_static_cast<uint32_t, size_t>(samplers_.size());
	rootDesc.pStaticSamplers = reinterpret_cast<const D3D12_STATIC_SAMPLER_DESC*>(samplers_.data());
	rootDesc.Flags = flags;


	ID3DBlob* pOutBlob;
	ID3DBlob* pErrorBlob;

	HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob);
	if (FAILED(hr)) {
		X_FATAL("Dx12", "Failed to serialize root signature: %" PRIu32, hr);
		return;
	}

	hr = pDevice->CreateRootSignature(1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&pSignature_));
	if (FAILED(hr)) {
		X_FATAL("Dx12", "Failed to create root signature: %" PRIu32, hr);
	}

	D3DDebug::SetDebugObjectName(pSignature_, L"RootSignature");
}


X_NAMESPACE_END