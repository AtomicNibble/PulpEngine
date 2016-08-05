#include "stdafx.h"
#include "PipelineState.h"

#include "RootSignature.h"

X_NAMESPACE_BEGIN(render)


namespace
{

	struct CD3D12_SHADER_BYTECODE : public D3D12_SHADER_BYTECODE
	{
		CD3D12_SHADER_BYTECODE()
		{
		}

		explicit CD3D12_SHADER_BYTECODE(const D3D12_SHADER_BYTECODE& o) :
			D3D12_SHADER_BYTECODE(o)
		{
		}

		explicit CD3D12_SHADER_BYTECODE(const void* _pShaderBytecode, SIZE_T _BytecodeLength)
		{
			pShaderBytecode = _pShaderBytecode;
			BytecodeLength = _BytecodeLength;
		}

		~CD3D12_SHADER_BYTECODE() 
		{
		}

		operator const D3D12_SHADER_BYTECODE&() const { 
			return *this; 
		}
	};


} // namespace


PSODeviceCache::PSODeviceCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
	pDevice_(pDevice),
	cache_(arena, 32)
{

}

PSODeviceCache::~PSODeviceCache()
{
	destoryAll();
}

void PSODeviceCache::destoryAll(void)
{
	core::CriticalSection::ScopedLock lock(cacheLock_);

	auto it = cache_.begin();
	for (; it != cache_.end(); ++it) {
		it->second->Release();
	}

	cache_.clear();
}

bool PSODeviceCache::compile(D3D12_GRAPHICS_PIPELINE_STATE_DESC& gpsoDesc, const GraphicsPSO::InputLayoutArr& il, 
	ID3D12PipelineState** pPSO)
{
	X_ASSERT(gpsoDesc.InputLayout.NumElements == il.size(), "Input element size mismatch")(gpsoDesc.InputLayout.NumElements, il.size());

	HashVal hash = getHash(gpsoDesc, il);

	// set after calculating hash.
	gpsoDesc.InputLayout.pInputElementDescs = il.data();

	ID3D12PipelineState** pPSORef = nullptr;
	bool notCompiled = false;
	{
		core::CriticalSection::ScopedLock lock(cacheLock_);

		auto it = cache_.find(hash);
		if (it != cache_.end()) {
			// get ref of hash location.
			// which might have a value of null.
			pPSORef = &it->second;
		}
		else {
			notCompiled = true;
			// insert the entry for this hash, before we start compile.
			// that way we don't have to lock for whole compile time.
			auto insertIt = cache_.insert(std::make_pair(hash, *pPSO));

			pPSORef = &insertIt.first->second;
		}
	}

	if(notCompiled)
	{
		HRESULT hr = pDevice_->CreateGraphicsPipelineState(&gpsoDesc, IID_PPV_ARGS(pPSO));
		if (FAILED(hr)) {
			X_FATAL("Dx12", "Failed to create graphics PSO: %" PRIu32, hr);
		}

		*pPSORef = *pPSO;
	}
	else
	{
		// if might of been inserted but not finished compiling, so wait.
		while (*pPSORef == nullptr) {
			core::Thread::Yield();
		}
		*pPSO = *pPSORef;
	}

	return true;
}

bool PSODeviceCache::compile(D3D12_COMPUTE_PIPELINE_STATE_DESC& cpsoDesc, ID3D12PipelineState** pPSO)
{
	HashVal hash = getHash(cpsoDesc);

	ID3D12PipelineState** pPSORef = nullptr;
	bool notCompiled = false;
	{
		core::CriticalSection::ScopedLock lock(cacheLock_);

		auto it = cache_.find(hash);
		if (it != cache_.end()) {
			*pPSO = it->second;
			return true;
		}
		else {
			notCompiled = true;
			auto insertIt = cache_.insert(std::make_pair(hash, *pPSO));
			pPSORef = &insertIt.first->second;
		}
	}

	if (notCompiled)
	{
		HRESULT hr = pDevice_->CreateComputePipelineState(&cpsoDesc, IID_PPV_ARGS(pPSO));
		if (FAILED(hr)) {
			X_FATAL("Dx12", "Failed to create compute PSO: %" PRIu32, hr);
		}

		*pPSORef = *pPSO;
	}
	else
	{
		while (*pPSORef == nullptr) {
			core::Thread::Yield();
		}
		*pPSO = *pPSORef;
	}

	return true;
}

PSODeviceCache::HashVal PSODeviceCache::getHash(D3D12_GRAPHICS_PIPELINE_STATE_DESC& gpsoDesc, const GraphicsPSO::InputLayoutArr& il)
{
	core::Hash::xxHash64 hasher;

	// we don't want to include the pointer in the hash.
	// we have root sig pointer but that's ok.
	gpsoDesc.InputLayout.pInputElementDescs = nullptr;

	hasher.reset(0x52652);
	hasher.update(&gpsoDesc, 1);
	hasher.update(il.data(), il.size());

	return hasher.finalize();
}

PSODeviceCache::HashVal PSODeviceCache::getHash(D3D12_COMPUTE_PIPELINE_STATE_DESC& cpsoDesc)
{
	core::Hash::xxHash64 hasher;
	
	hasher.reset(0x79371); // diffrent seed to graphics desc.
	hasher.update(&cpsoDesc, 1);

	return hasher.finalize();
}

// --------------------------------------------------

void PSO::setRootSignature(const RootSignature& BindMappings)
{
	pRootSignature_ = &BindMappings;
}


// --------------------------------------------------


GraphicsPSO::GraphicsPSO(core::MemoryArenaBase* arena) :
	inputLayouts_(arena)
{
	core::zero_object(PSODesc_);
	PSODesc_.NodeMask = 1;
	PSODesc_.SampleMask = 0xFFFFFFFFu;
	PSODesc_.SampleDesc.Count = 1;
	PSODesc_.InputLayout.NumElements = 0;
}

GraphicsPSO::~GraphicsPSO()
{

}


void GraphicsPSO::finalize(PSODeviceCache& cache)
{
	// Make sure the root signature is finalized first
	PSODesc_.pRootSignature = pRootSignature_->getSignature();

	X_ASSERT(PSODesc_.pRootSignature != nullptr, "root signature must be finalized before finalize PSO")();

	cache.compile(PSODesc_, inputLayouts_, &pPSO_);
}


void GraphicsPSO::setBlendState(const D3D12_BLEND_DESC& blendDesc)
{
	PSODesc_.BlendState = blendDesc;
}

void GraphicsPSO::setRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
{
	PSODesc_.RasterizerState = rasterizerDesc;
}

void GraphicsPSO::setDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
	PSODesc_.DepthStencilState = depthStencilDesc;
}

void GraphicsPSO::setSampleMask(uint32_t sampleMask)
{
	PSODesc_.SampleMask = sampleMask;
}

void GraphicsPSO::setPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	PSODesc_.PrimitiveTopologyType = topologyType;
}

void GraphicsPSO::setRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat,
	uint32_t msaaCount, uint32_t msaaQuality)
{
	setRenderTargetFormats(1, &RTVFormat, DSVFormat, msaaCount, msaaQuality);
}

void GraphicsPSO::setRenderTargetFormats(uint32_t numRTVs, const DXGI_FORMAT* pRTVFormats,
	DXGI_FORMAT DSVFormat, uint32_t MsaaCount , uint32_t MsaaQuality)
{
	X_ASSERT_NOT_NULL(pRTVFormats);

	for (uint32_t i = 0; i < numRTVs; ++i) {
		PSODesc_.RTVFormats[i] = pRTVFormats[i];
	}
	for (uint32_t i = numRTVs; i < PSODesc_.NumRenderTargets; ++i) {
		PSODesc_.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}

	PSODesc_.NumRenderTargets = numRTVs;
	PSODesc_.DSVFormat = DSVFormat;
	PSODesc_.SampleDesc.Count = MsaaCount;
	PSODesc_.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::setInputLayout(uint32_t numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
{
	PSODesc_.InputLayout.NumElements = numElements;

	if (numElements > 0)
	{
		X_ASSERT_NOT_NULL(pInputElementDescs);

		inputLayouts_.resize(numElements);

		memcpy(inputLayouts_.data(), pInputElementDescs, numElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
	} 
	else {
		inputLayouts_.clear();
	}
}

void GraphicsPSO::setPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
	PSODesc_.IBStripCutValue = IBProps;
}



void GraphicsPSO::setVertexShader(const void* pBinary, size_t Size)
{
	PSODesc_.VS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

void GraphicsPSO::setPixelShader(const void* pBinary, size_t Size)
{
	PSODesc_.PS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

void GraphicsPSO::setGeometryShader(const void* pBinary, size_t Size)
{
	PSODesc_.GS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

void GraphicsPSO::setHullShader(const void* pBinary, size_t Size)
{
	PSODesc_.HS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

void GraphicsPSO::setDomainShader(const void* pBinary, size_t Size)
{
	PSODesc_.DS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}


// ----------------------------------------

void ComputePSO::finalize(PSODeviceCache& cache)
{
	// Make sure the root signature is finalized first
	PSODesc_.pRootSignature = pRootSignature_->getSignature();

	X_ASSERT(PSODesc_.pRootSignature != nullptr, "root signature must be finalized before finalize PSO")();

	cache.compile(PSODesc_, &pPSO_);
}


void ComputePSO::setComputeShader(const void* pBinary, size_t size)
{
	PSODesc_.CS = CD3D12_SHADER_BYTECODE(pBinary, size);
}



X_NAMESPACE_END