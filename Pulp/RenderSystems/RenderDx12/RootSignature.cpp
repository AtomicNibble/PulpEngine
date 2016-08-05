#include "stdafx.h"
#include "RootSignature.h"

X_NAMESPACE_BEGIN(render)

RootSignatureDeviceCache::RootSignatureDeviceCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
	pDevice_(pDevice),
	cache_(arena, 32)
{

}

RootSignatureDeviceCache::~RootSignatureDeviceCache()
{
	destoryAll();
}

void RootSignatureDeviceCache::destoryAll(void)
{
	auto it = cache_.begin();
	for (; it != cache_.end(); ++it) {
		it->second->Release();
	}

	cache_.clear();
}

bool RootSignatureDeviceCache::compile(D3D12_ROOT_SIGNATURE_DESC& rootDesc, D3D12_ROOT_SIGNATURE_FLAGS flags, 
	ID3D12RootSignature** pSignature)
{
	X_ASSERT_NOT_NULL(pSignature);

	HashVal hash = getHash(rootDesc, flags);

	auto it = cache_.find(hash);
	if (it != cache_.end()) {
		*pSignature = it->second;
	}

	{
		ID3DBlob* pOutBlob;
		ID3DBlob* pErrorBlob;

		HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob);
		if (FAILED(hr)) {
			const char* pErr = "<na>";
			if (pErrorBlob) {
				pErr = static_cast<const char*>(pErrorBlob->GetBufferPointer());
			}

			X_FATAL("Dx12", "Failed to serialize root signature: %" PRIu32 " Err: \"%s\"", hr, pErr);
			core::SafeReleaseDX(pOutBlob);
			core::SafeReleaseDX(pErrorBlob);
			return false;
		}

		hr = pDevice_->CreateRootSignature(1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(pSignature));
		if (FAILED(hr)) {
			X_FATAL("Dx12", "Failed to create root signature: %" PRIu32, hr);
		}

		D3DDebug::SetDebugObjectName(*pSignature, L"RootSignature");


		cache_.insert(std::make_pair(hash, *pSignature));

		core::SafeReleaseDX(pOutBlob);
		core::SafeReleaseDX(pErrorBlob);
	}
	return true;
}

RootSignatureDeviceCache::HashVal RootSignatureDeviceCache::getHash(D3D12_ROOT_SIGNATURE_DESC& rootDesc,
	D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	core::Hash::xxHash64 hasher;

 	hasher.update(rootDesc.pStaticSamplers, rootDesc.NumStaticSamplers);

	for (uint32_t param = 0; param < rootDesc.NumParameters; ++param)
	{
		const auto& rootParam = rootDesc.pParameters[param];

		if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			X_ASSERT_NOT_NULL(rootParam.DescriptorTable.pDescriptorRanges);

			hasher.update(rootParam.DescriptorTable.pDescriptorRanges, rootParam.DescriptorTable.NumDescriptorRanges);

			// We don't care about sampler descriptor tables.  We don't cache them
			if (rootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
				continue;
			}

			// ...
		}
		else
		{
			hasher.update(&rootParam, 1);
		}
	}

	// include flags?
	hasher.update(&flags, 1);

	return hasher.finalize();
}

// --------------------------------------------------------

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

void RootSignature::finalize(RootSignatureDeviceCache& cache, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	if (pSignature_) {
		X_WARNING("Dx12", "finalize called on a rootSig that already has a device object");
		return;
	}

	X_ASSERT(samplesInitCount_ == static_cast<uint32_t>(samplers_.size()), "Not all samplers are init")(samplesInitCount_, samplers_.size());

	D3D12_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.NumParameters = safe_static_cast<uint32_t, size_t>(params_.size());
	rootDesc.pParameters = reinterpret_cast<const D3D12_ROOT_PARAMETER*>(params_.data());
	rootDesc.NumStaticSamplers = safe_static_cast<uint32_t, size_t>(samplers_.size());
	rootDesc.pStaticSamplers = reinterpret_cast<const D3D12_STATIC_SAMPLER_DESC*>(samplers_.data());
	rootDesc.Flags = flags;

#if X_DEBUG
	// check we below 64 DWORDS.
	uint32_t numDwords = 0;
	for (const auto& param : params_)
	{
		const D3D12_ROOT_PARAMETER& rp = param();

		switch (param.getType())
		{
		case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			++numDwords;
			break;
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			numDwords += rp.Constants.Num32BitValues;
			break;
		case D3D12_ROOT_PARAMETER_TYPE_CBV:
		case D3D12_ROOT_PARAMETER_TYPE_SRV:
		case D3D12_ROOT_PARAMETER_TYPE_UAV:
			numDwords += 2;
			break;
		default:
			X_ASSERT_UNREACHABLE();
			break;
		}
	}

	if (numDwords > MAX_DWORDS) {
		X_FATAL("Dx12", "rootSig parameters size exceeds limit. num: %" PRIu32, " max: %" PRIuS, numDwords, MAX_DWORDS );
		return;
	}

#endif // !X_DEBUG


	if (!cache.compile(rootDesc, flags, &pSignature_)) {
		X_ERROR("Dx12", "Failed to compile root sig");
	}
}


X_NAMESPACE_END