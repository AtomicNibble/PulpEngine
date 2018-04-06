#pragma once

#include <Containers\HashMap.h>

#include <Hashing\xxHash.h>

X_NAMESPACE_BEGIN(render)

class DescriptorCache;

// makes use of g_rendererArena
class RootParameter
{
public:
    X_INLINE explicit RootParameter();
    X_INLINE ~RootParameter();

    X_INLINE void clear(void);

    X_INLINE void initAsConstants(uint32_t shaderRegister, uint32_t NumDwords, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    X_INLINE void initAsCBV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    X_INLINE void initAsSRV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    X_INLINE void initAsUAV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    // this is just a helper for calling initAsDescriptorTable and setTableRange
    X_INLINE void initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t baseShaderRegister,
        uint32_t count, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    // You can init as a desc table with 3 ranges, then you will have to call setTableRange on index 0,1,3 setting the type register and count.
    // This is needed if you want to create a Descriptor heap with more than one D3D12_DESCRIPTOR_RANGE_TYPE
    // You need to state what range of the heap each type takes up by setting the count.
    // and they must appear in order.
    // Eg: ranges of: SRV:256 CBV:32 UAV:6
    // result in the following starting indexs for each type: SRV:256 CBV:288 UAV:284
    X_INLINE void initAsDescriptorTable(uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    X_INLINE void setTableRange(uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type,
        uint32_t baseShaderRegister, uint32_t Count,
        // I don't think i will make use of this. it's for shit like: register(t3,space5); which means hlsl code needs to target specific register space.
        uint32_t space = 0);

    X_INLINE const D3D12_ROOT_PARAMETER& operator()(void) const;
    X_INLINE D3D12_ROOT_PARAMETER_TYPE getType(void) const;

protected:
    // 1:1 mapping of D3D12_ROOT_PARAMETER, so can't add any extra members.
    D3D12_ROOT_PARAMETER rootParam_;
};

static_assert(sizeof(RootParameter) == sizeof(D3D12_ROOT_PARAMETER), "rootParam must have same size as d3d type");

class RootSignatureDeviceCache
{
public:
    typedef core::Hash::xxHash64::HashVal HashVal;

private:
    typedef core::HashMap<HashVal, ID3D12RootSignature*> SigMap;

public:
    RootSignatureDeviceCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice);
    ~RootSignatureDeviceCache();

    void destoryAll(void);

    bool compile(D3D12_ROOT_SIGNATURE_DESC& rootDesc, RootSignatureDeviceCache::HashVal hash,
        D3D12_ROOT_SIGNATURE_FLAGS flags, ID3D12RootSignature** pSignature);

private:
    core::CriticalSection cacheLock_;
    ID3D12Device* pDevice_;
    SigMap cache_;
};

class RootSignature
{
public:
    // Maximum 64 DWORDS divied up amongst all root parameters. (Dx12 limit)
    // Root constants = 1 DWORD * NumConstants
    // Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
    // Descriptor table pointer = 1 DWORD
    // Static samplers = 0 DWORDS (compiled into shader)

    static const size_t MAX_DWORDS = 64;

public:
    X_INLINE RootSignature(core::MemoryArenaBase* arena, size_t numRootParams = 0, size_t numStaticSamplers = 0);
    X_INLINE ~RootSignature();

    // free's all the cpu param descriptions.
    // safe todo after finalize.
    void freeParams(void);
    X_INLINE void reset(size_t numRootParams, size_t numStaticSamplers = 0);

    void initStaticSampler(uint32_t shaderRegister, const D3D12_SAMPLER_DESC& nonStaticSamplerDesc,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    bool finalize(RootSignatureDeviceCache& cache, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

    X_INLINE size_t numParams(void) const;
    X_INLINE RootParameter& getParamRef(size_t idx);
    X_INLINE const RootParameter& getParamRef(size_t idx) const;
    X_INLINE ID3D12RootSignature* getSignature(void) const;

    X_INLINE uint32_t descriptorTableBitMap(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
    X_INLINE uint32_t descriptorTableSize(size_t idx) const;

private:
    RootSignatureDeviceCache::HashVal gethashAndPopulateDescriptorTableMap(D3D12_ROOT_SIGNATURE_DESC& rootDesc,
        D3D12_ROOT_SIGNATURE_FLAGS flags);

protected:
    core::Array<RootParameter> params_;
    core::Array<D3D12_STATIC_SAMPLER_DESC> samplers_;

    uint32_t descriptorTableBitMap_;         // One bit is set for root parameters that are (non-sampler) descriptor tables
    uint32_t descriptorTableSamplerBitMap_;  // One bit is set for root parameters that are (sampler) descriptor tables
    uint32_t descriptorTableSize_[16];       // includes both sampler/none-sampeler descriptor tables need to know their descriptor count
    uint32_t maxDescriptorCacheHandleCount_; // The sum of all non-sampler descriptor table counts

    uint32_t samplesInitCount_;
    ID3D12RootSignature* pSignature_;
};

X_NAMESPACE_END

#include "RootSignature.inl"