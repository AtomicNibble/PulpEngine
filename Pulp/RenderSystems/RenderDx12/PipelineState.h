#pragma once

#include <Containers\HashMap.h>
#include <Hashing\xxHash.h>

X_NAMESPACE_BEGIN(render)

class RootSignature;
class PSODeviceCache;

class PSO
{
public:
    X_INLINE PSO();
    X_INLINE ~PSO();

    X_INLINE bool isRootSigSet(void) const;
    X_INLINE bool isDeviceObjectValid(void) const;

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
    X_INLINE GraphicsPSO();
    X_INLINE ~GraphicsPSO();

    bool finalize(PSODeviceCache& cache);

    X_INLINE void setBlendState(const D3D12_BLEND_DESC& blendDesc);
    X_INLINE void setRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc);
    X_INLINE void setDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);
    X_INLINE void setSampleMask(uint32_t sampleMask);
    X_INLINE void setPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
    X_INLINE void setRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, uint32_t msaaCount = 1, uint32_t msaaQuality = 0);
    void setRenderTargetFormats(uint32_t numRTVs, const DXGI_FORMAT* pRTVFormats,
        DXGI_FORMAT DSVFormat, uint32_t MsaaCount = 1, uint32_t MsaaQuality = 0);
    X_INLINE void setPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

    // you must ensure this memory remains valid untill finalize is called.
    // this is becuase pretty much all these will come from the IL cache which is built on startup.
    void setInputLayout(size_t numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);

    X_INLINE void setVertexShader(const void* pBinary, size_t Size);
    X_INLINE void setPixelShader(const void* pBinary, size_t Size);
    X_INLINE void setGeometryShader(const void* pBinary, size_t Size);
    X_INLINE void setHullShader(const void* pBinary, size_t Size);
    X_INLINE void setDomainShader(const void* pBinary, size_t Size);

    X_INLINE void setVertexShader(const D3D12_SHADER_BYTECODE& binary);
    X_INLINE void setPixelShader(const D3D12_SHADER_BYTECODE& binary);
    X_INLINE void setGeometryShader(const D3D12_SHADER_BYTECODE& binary);
    X_INLINE void setHullShader(const D3D12_SHADER_BYTECODE& binary);
    X_INLINE void setDomainShader(const D3D12_SHADER_BYTECODE& binary);

    // Perform validation and compute a hash value for fast state block comparisons

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc_;
};

class ComputePSO : public PSO
{
public:
    X_INLINE ComputePSO();
    X_INLINE ~ComputePSO();

    bool finalize(PSODeviceCache& cache);

    void setComputeShader(const void* pBinary, size_t size);
    X_INLINE void setComputeShader(const D3D12_SHADER_BYTECODE& binary);

private:
    D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc_;
};

class PSODeviceCache
{
    typedef core::Hash::xxHash64::HashVal HashVal;
    typedef core::HashMap<HashVal, ID3D12PipelineState*> PSOMap;

    static const uintptr_t INVALID_PSO = std::numeric_limits<uintptr_t>::max();

public:
    PSODeviceCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice);
    ~PSODeviceCache();

    void registerVars(void);
    void destoryAll(void);

    bool compile(D3D12_GRAPHICS_PIPELINE_STATE_DESC& gpsoDesc, ID3D12PipelineState** pPSO);
    bool compile(D3D12_COMPUTE_PIPELINE_STATE_DESC& cpsoDesc, ID3D12PipelineState** pPSO);

private:
    static HashVal getHash(D3D12_GRAPHICS_PIPELINE_STATE_DESC& gpsoDesc);
    static HashVal getHash(D3D12_COMPUTE_PIPELINE_STATE_DESC& cpsoDesc);

private:
    core::CriticalSection cacheLock_;
    ID3D12Device* pDevice_;
    PSOMap cache_;

    int32_t helpWithWorkonPSOStall_;
};

X_NAMESPACE_END

#include "PipelineState.inl"