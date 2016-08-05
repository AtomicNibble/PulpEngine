#pragma once

X_NAMESPACE_BEGIN(render)

class RootSignature;

class PSO
{
public:
	X_INLINE PSO();
	X_INLINE ~PSO();


	void setRootSignature(const RootSignature& BindMappings);
	X_INLINE const RootSignature& getRootSignature(void) const;

	X_INLINE ID3D12PipelineState* getPipelineStateObject(void) const;

protected:
	const RootSignature* pRootSignature_;
	ID3D12PipelineState* pPSO_;
};



class GraphicsPSO : public PSO
{
public:
	X_INLINE GraphicsPSO(core::MemoryArenaBase* arena);
	X_INLINE ~GraphicsPSO();

	void finalize(ID3D12Device* pDevice);

	void setBlendState(const D3D12_BLEND_DESC& blendDesc);
	void setRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc);
	void setDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);
	void setSampleMask(uint32_t sampleMask);
	void setPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
	void setRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, uint32_t msaaCount = 1, uint32_t msaaQuality = 0);
	void setRenderTargetFormats(uint32_t numRTVs, const DXGI_FORMAT* pRTVFormats,
		DXGI_FORMAT DSVFormat, uint32_t MsaaCount = 1, uint32_t MsaaQuality = 0);
	void setInputLayout(uint32_t numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
	void setPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

	void setVertexShader(const void* pBinary, size_t Size);
	void setPixelShader(const void* pBinary, size_t Size);
	void setGeometryShader(const void* pBinary, size_t Size);
	void setHullShader(const void* pBinary, size_t Size);
	void setDomainShader(const void* pBinary, size_t Size);
	
	X_INLINE void setVertexShader(const D3D12_SHADER_BYTECODE& binary);
	X_INLINE void setPixelShader(const D3D12_SHADER_BYTECODE& binary);
	X_INLINE void setGeometryShader(const D3D12_SHADER_BYTECODE& binary);
	X_INLINE void setHullShader(const D3D12_SHADER_BYTECODE& binary);
	X_INLINE void setDomainShader(const D3D12_SHADER_BYTECODE& binary);

	// Perform validation and compute a hash value for fast state block comparisons

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc_;
	core::Array<D3D12_INPUT_ELEMENT_DESC> inputLayouts_;
};



class ComputePSO : public PSO
{
public:
	X_INLINE ComputePSO();
	X_INLINE ~ComputePSO();

	void finalize(ID3D12Device* pDevice);

	void setComputeShader(const void* pBinary, size_t size);
	X_INLINE void setComputeShader(const D3D12_SHADER_BYTECODE& binary);

private:

	D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc_;
};


X_NAMESPACE_END

#include "PipelineState.inl"