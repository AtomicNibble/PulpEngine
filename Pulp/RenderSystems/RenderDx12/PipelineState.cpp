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


void GraphicsPSO::finalize(ID3D12Device* pDevice)
{
	// Make sure the root signature is finalized first
	PSODesc_.pRootSignature = pRootSignature_->getSignature();

	X_ASSERT(PSODesc_.pRootSignature != nullptr, "root signature must be finalized before finalize PSO")();

	PSODesc_.InputLayout.pInputElementDescs = nullptr;
	PSODesc_.InputLayout.pInputElementDescs = inputLayouts_.data();

	HRESULT hr = pDevice->CreateGraphicsPipelineState(&PSODesc_, IID_PPV_ARGS(&pPSO_));
	if (FAILED(hr)) {
		X_FATAL("Dx12", "Failed to create root signature: %" PRIu32, hr);
	}
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



X_NAMESPACE_END