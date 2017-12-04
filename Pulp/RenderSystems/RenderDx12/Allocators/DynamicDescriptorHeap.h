#pragma once

#include "DescriptorAllocator.h" // DescriptorHandle
#include <Containers\Fifo.h>

X_NAMESPACE_BEGIN(render)

/*

DynamicDescriptorHeap:
UseCase: Setting descriptor heaps, between each draw.
Cleanup: ?

Details:
	This is a linear allocator for dynamic generated descriptor tables.

	See below..
*/

class RootSignature;
class CommandContext;
class CommandListManger;

class DescriptorTypeAllocatorPool
{
public:
	static const uint32_t NUM_DESCRIPTORS_PER_HEAP[DescriptorHeapType::ENUM_COUNT];

public:
	DescriptorTypeAllocatorPool(core::MemoryArenaBase* arena, ID3D12Device* pDevice, CommandListManger& commandManager, D3D12_DESCRIPTOR_HEAP_TYPE type);
	~DescriptorTypeAllocatorPool();

	void destoryAll(void);

	ID3D12DescriptorHeap* requestDescriptorHeap(void);
	void discardDescriptorHeaps(uint64_t fenceValueForReset, const core::Array<ID3D12DescriptorHeap*>& usedHeaps);

	D3D12_DESCRIPTOR_HEAP_TYPE getType(void) const;


private:
	D3D12_DESCRIPTOR_HEAP_TYPE type_;
	ID3D12Device* pDevice_;
	CommandListManger& commandManager_;

	core::CriticalSection cs_;
	core::Fifo<std::pair<uint64_t, ID3D12DescriptorHeap*>> retiredDescriptorHeaps_;
	core::Fifo<ID3D12DescriptorHeap*> availableDescriptorHeaps_;
	core::Array<ID3D12DescriptorHeap*> descriptorHeapPool_;
};


class DescriptorAllocatorPool
{
public:

public:
	DescriptorAllocatorPool(core::MemoryArenaBase* arena, ID3D12Device* pDevice, CommandListManger& commandManager);
	~DescriptorAllocatorPool() = default;

	void destoryAll(void);

	ID3D12DescriptorHeap* requestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
	void discardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE type, uint64_t fenceValueForReset, const core::Array<ID3D12DescriptorHeap*>& usedHeaps);

private:

	std::array<DescriptorTypeAllocatorPool, 2> allocators_;
};


//
// This is a linear allocator for dynamic generated descriptor tables.
// It internally caches the handles so can move to a new descriptor heap if we run out of room.
//
// The class requires the rootSig to be set so we can get info on what rootIdx are descriptor tables and create caches for each.
// We have a 'DescriptorTableCache' for each rootIdx that is a descriptor table.
//
// Note this class ignores sampler descriptorTables..
//
// We have a 'DescriptorHandleCache' for compute and one for graphics.
// Each one has support for MAXNUM_DESCRIPTOR_TABLES descriptor tables with a max root index of MAXNUM_DESCRIPTOR_TABLES.
// MAXNUM_DESCRIPTORS is the max descriptors across all descriptor ranges.
// each DescriptorTableCache places it's cache in this single buffer.
// 
// 
// calling 'setGraphicsDescriptorHandles' copy's the descriptors into to cpu cache for that rootIdx and marks the rootIdx as stale.
// it does not perform redudancy checks currently but could.
// 
// calling commitGraphicsRootDescriptorTables is no-op if no params are stale aka no calls to 'setGraphicsDescriptorHandles' since last time.
// when there are stale params it first work out how much space is needed to store all descriptors for each stale rootIdx.
// if the current bound descriptorHeap is full we allocate a new one and mark all params as stale.
// next we ensure the decriptor heap is correct for the current context.
// now for each rootIDx that is stale we update the rootIdx descriptor table pointer by calling 'ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable'
// then it proceesed to upload the cpu descriptor handles to the dscriptorheap in batches of MAX_DESCRIPTORS_PER_COPY by calling  pDevice->CopyDescriptors
//
//
//
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
		static const uint32_t MAXNUM_DESCRIPTORS = 64;
		static const uint32_t MAX_DESCRIPTORS_PER_COPY = 16; // max we send to device::CopyDescriptors at once.

	public:
		DescriptorHandleCache();

		void clearCache(void);

		uint32_t computeStagedSize(void);
		void copyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12Device* pDevice, DescriptorHandle DestHandleStart,
			uint32_t descriptorSize, ID3D12GraphicsCommandList* pCmdList, SetRootDescriptorfunctionPtr pSetFunc);


		void unbindAllValid(void);
		void stageDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);
		void parseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig);

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
	DynamicDescriptorHeap(core::MemoryArenaBase* arena, ID3D12Device* pDevice, DescriptorAllocatorPool& pool,
		CommandContext& owningContext, D3D12_DESCRIPTOR_HEAP_TYPE type);

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
	D3D12_DESCRIPTOR_HEAP_TYPE type_;
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