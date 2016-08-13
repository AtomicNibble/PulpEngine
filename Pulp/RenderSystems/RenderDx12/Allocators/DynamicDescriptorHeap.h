#pragma once

#include "DescriptorAllocator.h" // DescriptorHandle
#include <Containers\Fifo.h>

X_NAMESPACE_BEGIN(render)

class RootSignature;
class CommandContext;
class CommandListManger;

class DescriptorAllocatorPool
{
public:
	static const uint32_t NUM_DESCRIPTORS_PER_HEAP = 1024;

public:
	DescriptorAllocatorPool(core::MemoryArenaBase* arena, ID3D12Device* pDevice, CommandListManger& commandManager);
	~DescriptorAllocatorPool();

	ID3D12DescriptorHeap* requestDescriptorHeap(void);
	void discardDescriptorHeaps(uint64_t fenceValueForReset, const core::Array<ID3D12DescriptorHeap*>& usedHeaps);

private:
	ID3D12Device* pDevice_;
	CommandListManger& commandManager_;

	core::CriticalSection cs_;
	core::Fifo<std::pair<uint64_t, ID3D12DescriptorHeap*>> retiredDescriptorHeaps_;
	core::Fifo<ID3D12DescriptorHeap*> availableDescriptorHeaps_;
	core::Array<ID3D12DescriptorHeap*> descriptorHeapPool_;
};

class DynamicDescriptorHeap
{
	typedef core::traits::MemberFunctionStd<ID3D12GraphicsCommandList, 
		void(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE)>::Pointer SetRootDescriptorfunctionPtr;
private:

	// Describes a descriptor table entry:  a region of the handle cache and which handles have been set
	struct DescriptorTableCache
	{
		DescriptorTableCache();

		uint32_t assignedHandlesBitMap;
		D3D12_CPU_DESCRIPTOR_HANDLE* pTableStart;
		uint32_t tableSize;
	};

	struct DescriptorHandleCache
	{
		static const uint32_t MAXNUM_DESCRIPTOR_TABLES = 16;
		static const uint32_t MAXNUM_DESCRIPTORS = 256;
		static const uint32_t MAX_DESCRIPTORS_PER_COPY = 16;

	public:
		DescriptorHandleCache();

		void clearCache(void);

		uint32_t computeStagedSize(void);
		void copyAndBindStaleTables(ID3D12Device* pDevice, DescriptorHandle DestHandleStart, 
			uint32_t descriptorSize, ID3D12GraphicsCommandList* pCmdList, SetRootDescriptorfunctionPtr pSetFunc);


		void unbindAllValid(void);
		void stageDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);
		void parseRootSignature(const RootSignature& rootSig);

		X_INLINE uint32_t rootDescriptorTablesBitMap(void) const;
		X_INLINE uint32_t staleRootParamsBitMap(void) const;
		X_INLINE uint32_t maxCachedDescriptors(void) const;

	private:
		uint32_t rootDescriptorTablesBitMap_;
		uint32_t staleRootParamsBitMap_;
		uint32_t maxCachedDescriptors_;

		DescriptorTableCache rootDescriptorTable_[MAXNUM_DESCRIPTOR_TABLES];
		D3D12_CPU_DESCRIPTOR_HANDLE hHandleCache_[MAXNUM_DESCRIPTORS];
	};


public:
	DynamicDescriptorHeap(core::MemoryArenaBase* arena, ID3D12Device* pDevice, DescriptorAllocatorPool& pool, CommandContext& owningContext);
	~DynamicDescriptorHeap();


	void cleanupUsedHeaps(uint64_t fenceValue);

	// Copy multiple handles into the cache area reserved for the specified root parameter.
	void setGraphicsDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);
	void setComputeDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);

	// Bypass the cache and upload directly to the shader-visible heap
	D3D12_GPU_DESCRIPTOR_HANDLE uploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle);

	// Deduce cache layout needed to support the descriptor tables needed by the root signature.
	void parseGraphicsRootSignature(const RootSignature& rootSig);
	void parseComputeRootSignature(const RootSignature& rootSig);

	// Upload any new descriptors in the cache to the shader-visible heap.
	X_INLINE void commitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* pCmdList);
	X_INLINE void commitComputeRootDescriptorTables(ID3D12GraphicsCommandList* pCmdList);


private:
	bool hasSpace(uint32_t count) const;
	void retireCurrentHeap(void);
	void retireUsedHeaps(uint64_t fenceValue);
	ID3D12DescriptorHeap* getHeapPointer(void);

	DescriptorHandle allocate(uint32_t count);

	void copyAndBindStagedTables(DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* pCmdList, 
		SetRootDescriptorfunctionPtr pSetFunc);

	// Mark all descriptors in the cache as stale and in need of re-uploading.
	void unbindAllValid(void);

	uint32_t getDescriptorSize(void);


private:
	ID3D12Device* pDevice_;

	DescriptorHandleCache graphicsHandleCache_;
	DescriptorHandleCache computeHandleCache_;

	DescriptorAllocatorPool& pool_;
	CommandContext& owningContext_;

	ID3D12DescriptorHeap* pCurrentHeapPtr_;
	uint32_t currentOffset_;
	DescriptorHandle firstDescriptor_;
	core::Array<ID3D12DescriptorHeap*> retiredHeaps_;

	uint32_t descriptorSize_;
};



X_NAMESPACE_END


#include "DynamicDescriptorHeap.inl"