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

	// lower case Register is reserved keyword.
	X_INLINE void initAsConstants(uint32_t Register, uint32_t NumDwords, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	X_INLINE void initAsConstantBuffer(uint32_t Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	X_INLINE void initAsBufferSRV(uint32_t Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	X_INLINE void initAsBufferUAV(uint32_t Register, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	X_INLINE void initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t Register,
		 	uint32_t Count, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	X_INLINE void initAsDescriptorTable(uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
	X_INLINE void setTableRange(uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type,
		uint32_t Register, uint32_t Count, uint32_t space = 0);
	 
	X_INLINE const D3D12_ROOT_PARAMETER& operator() (void) const;

	X_INLINE D3D12_ROOT_PARAMETER_TYPE getType(void) const;

protected:
	// 1:1 mapping of D3D12_ROOT_PARAMETER, so can't add any extra members.
	D3D12_ROOT_PARAMETER rootParam_;
};

class RootSignatureDeviceCache
{
	typedef core::Hash::xxHash64::HashVal HashVal;
	typedef core::HashMap<HashVal, ID3D12RootSignature* > SigMap;

public:
	RootSignatureDeviceCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice);
	~RootSignatureDeviceCache();

	void destoryAll(void);

	bool compile(D3D12_ROOT_SIGNATURE_DESC& rootDesc, D3D12_ROOT_SIGNATURE_FLAGS flags, ID3D12RootSignature** pSignature);
	
private:
	static HashVal getHash(D3D12_ROOT_SIGNATURE_DESC& rootDesc, D3D12_ROOT_SIGNATURE_FLAGS flags);

private:
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

	void clear(void); // keeps param memory.
	void free(void);
	X_INLINE void reset(size_t numRootParams, size_t numStaticSamplers = 0);

	void initStaticSampler(uint32_t Register, const D3D12_SAMPLER_DESC& nonStaticSamplerDesc,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

	void finalize(RootSignatureDeviceCache& cache, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	X_INLINE size_t numParams(void) const;
	X_INLINE RootParameter& getParamRef(size_t idx);
	X_INLINE const RootParameter& getParamRef(size_t idx) const;
	X_INLINE ID3D12RootSignature* getSignature(void) const;

	X_INLINE uint32_t descriptorTableBitMap(void) const;
	X_INLINE uint32_t descriptorTableSize(size_t idx) const;

protected:
	core::Array<RootParameter> params_;
	core::Array<D3D12_STATIC_SAMPLER_DESC> samplers_;

	uint32_t descriptorTableBitMap_;			// One bit is set for root parameters that are (non-sampler) descriptor tables
	uint32_t descriptorTableSize_[16];			// Non-sampler descriptor tables need to know their descriptor count
	uint32_t maxDescriptorCacheHandleCount_;	// The sum of all non-sampler descriptor table counts

	uint32_t samplesInitCount_;
	ID3D12RootSignature* pSignature_;
};



X_NAMESPACE_END


#include "RootSignature.inl"