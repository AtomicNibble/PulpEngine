#include "stdafx.h"
#include "DynamicDescriptorHeap.h"

#include "CommandList.h"
#include "CommandContex.h"
#include "RootSignature.h"

X_NAMESPACE_BEGIN(render)

DescriptorAllocatorPool::DescriptorAllocatorPool(core::MemoryArenaBase* arena, ID3D12Device* pDevice, CommandListManger& commandManager) :
	pDevice_(pDevice),
	commandManager_(commandManager),
	descriptorHeapPool_(arena)
{

}

DescriptorAllocatorPool::~DescriptorAllocatorPool()
{

}

ID3D12DescriptorHeap* DescriptorAllocatorPool::requestDescriptorHeap(void)
{
	core::CriticalSection::ScopedLock lock(cs_);

	while (!retiredDescriptorHeaps_.empty() && commandManager_.isFenceComplete(retiredDescriptorHeaps_.front().first))
	{
		availableDescriptorHeaps_.push(retiredDescriptorHeaps_.front().second);
		retiredDescriptorHeaps_.pop();
	}

	if (!availableDescriptorHeaps_.empty())
	{
		ID3D12DescriptorHeap* pHeapPtr = availableDescriptorHeaps_.front();
		availableDescriptorHeaps_.pop();
		return pHeapPtr;
	}
	else
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
		core::zero_object(heapDesc);
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NumDescriptors = NUM_DESCRIPTORS_PER_HEAP;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 1;

		ID3D12DescriptorHeap* pHeapPtr;
		HRESULT hr = pDevice_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pHeapPtr));
		if (FAILED(hr)) {
			X_FATAL("Dx12", "Failed to create descriptor heap. err: %" PRIu32, hr);
		}
		
		descriptorHeapPool_.emplace_back(pHeapPtr);
		return pHeapPtr;
	}
}

void DescriptorAllocatorPool::discardDescriptorHeaps(uint64_t FenceValue, const core::Array<ID3D12DescriptorHeap*>& UsedHeaps)
{
	core::CriticalSection::ScopedLock lock(cs_);

	for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter) {
		retiredDescriptorHeaps_.push(std::make_pair(FenceValue, *iter));
	}
}



// ---------------------------------------------------------------------

DynamicDescriptorHeap::DescriptorTableCache::DescriptorTableCache() :
	assignedHandlesBitMap(0), 
	pTableStart(nullptr),
	tableSize(0)
{

}

// ---------------------------------------------------------------------


DynamicDescriptorHeap::DescriptorHandleCache::DescriptorHandleCache()
{
	rootDescriptorTablesBitMap_ = 0;
	staleRootParamsBitMap_ = 0;
	maxCachedDescriptors_ = 0;
}

void DynamicDescriptorHeap::DescriptorHandleCache::clearCache(void)
{
	rootDescriptorTablesBitMap_ = 0;
	maxCachedDescriptors_ = 0;
}

uint32_t DynamicDescriptorHeap::DescriptorHandleCache::computeStagedSize(void)
{
	// Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
	uint32_t neededSpace = 0;
	uint32_t rootIndex;
	uint32_t staleParams = staleRootParamsBitMap_;

	while ((rootIndex = core::bitUtil::ScanBitsForward(staleParams)) != core::bitUtil::NO_BIT_SET)
	{
		staleParams ^= (1 << rootIndex);

		uint32_t maxSetHandle = core::bitUtil::ScanBits(rootDescriptorTable_[rootIndex].assignedHandlesBitMap);
		X_ASSERT(maxSetHandle != core::bitUtil::NO_BIT_SET, "Root entry masked as stable but has no stable descriptors")(maxSetHandle);

		neededSpace += maxSetHandle + 1;
	}

	return neededSpace;
}

void DynamicDescriptorHeap::DescriptorHandleCache::copyAndBindStaleTables(DescriptorHandle DestHandleStart, 
	ID3D12GraphicsCommandList* pCmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*pSetFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE))
{

}


void DynamicDescriptorHeap::DescriptorHandleCache::unbindAllValid(void)
{
	staleRootParamsBitMap_= 0;

	uint32_t tableParams = rootDescriptorTablesBitMap_;
	uint32_t rootIndex;
	
	while((rootIndex = core::bitUtil::ScanBitsForward(tableParams)) != core::bitUtil::NO_BIT_SET)
	{
		tableParams ^= (1 << rootIndex);
		
		if (rootDescriptorTable_[rootIndex].assignedHandlesBitMap != 0) {
			staleRootParamsBitMap_ |= (1 << rootIndex);
		}
	}
}

void DynamicDescriptorHeap::DescriptorHandleCache::stageDescriptorHandles(uint32_t rootIndex, uint32_t offset,
	uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
	X_ASSERT(((1 << rootIndex) & rootDescriptorTablesBitMap_) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table")();
	X_ASSERT(offset + numHandles <= rootDescriptorTable_[rootIndex].tableSize, "")();

	DescriptorTableCache& tableCache = rootDescriptorTable_[rootIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE* pCopyDest = tableCache.pTableStart + offset;
	for (UINT i = 0; i < numHandles; ++i) {
		pCopyDest[i] = pHandles[i];
	}

	tableCache.assignedHandlesBitMap |= ((1 << numHandles) - 1) << offset;
	staleRootParamsBitMap_ |= (1 << rootIndex);
}

void DynamicDescriptorHeap::DescriptorHandleCache::parseRootSignature(const RootSignature& rootSig)
{
	uint32_t CurrentOffset = 0;

	X_ASSERT(rootSig.numParams() <= 16, "Maybe we need to support something greater")(rootSig.numParams());

	staleRootParamsBitMap_ = 0;
	rootDescriptorTablesBitMap_ = rootSig.descriptorTableBitMap();

	uint32_t tableParams = rootDescriptorTablesBitMap_;
	uint32_t rootIndex;
	while ((rootIndex = core::bitUtil::ScanBitsForward(tableParams)) != core::bitUtil::NO_BIT_SET)
	{
		tableParams ^= (1 << rootIndex);

		uint32_t tableSize = rootSig.descriptorTableSize(rootIndex);
		X_ASSERT(tableSize > 0, "Table size must be none zero")(tableSize);

		DescriptorTableCache& RootDescriptorTable = rootDescriptorTable_[rootIndex];
		RootDescriptorTable.assignedHandlesBitMap = 0;
		RootDescriptorTable.pTableStart = hHandleCache_ + CurrentOffset;
		RootDescriptorTable.tableSize = tableSize;

		CurrentOffset += tableSize;
	}

	maxCachedDescriptors_ = CurrentOffset;

	X_ASSERT(maxCachedDescriptors_ <= MAXNUM_DESCRIPTORS, "Exceeded user-supplied maximum cache size")();
}



// ---------------------------------------------------------------------


DynamicDescriptorHeap::DynamicDescriptorHeap(core::MemoryArenaBase* arena, ID3D12Device* pDevice, 
		DescriptorAllocatorPool& pool, CommandContext& owningContext) :
	pDevice_(pDevice),
	retiredHeaps_(arena),
	owningContext_(owningContext),
	pool_(pool),
	pCurrentHeapPtr_(nullptr),
	currentOffset_(0)
{

}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{

}


void DynamicDescriptorHeap::cleanupUsedHeaps(uint64_t fenceValue)
{
	retireCurrentHeap();
	retireUsedHeaps(fenceValue);
	graphicsHandleCache_.clearCache();
	computeHandleCache_.clearCache();
}

void DynamicDescriptorHeap::setGraphicsDescriptorHandles(uint32_t rootIndex, uint32_t offset,
	uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
	graphicsHandleCache_.stageDescriptorHandles(rootIndex, offset, numHandles, pHandles);
}

void DynamicDescriptorHeap::setComputeDescriptorHandles(uint32_t rootIndex, uint32_t offset,
	uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
	computeHandleCache_.stageDescriptorHandles(rootIndex, offset, numHandles, pHandles);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::uploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	if (!hasSpace(1)) {
		retireCurrentHeap();
		unbindAllValid();
	}

	owningContext_.setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, getHeapPointer());

	DescriptorHandle destHandle = firstDescriptor_ + currentOffset_ * getDescriptorSize();
	currentOffset_ += 1;

	pDevice_->CopyDescriptorsSimple(1, destHandle.getCpuHandle(), handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return destHandle.getGpuHandle();
}

void DynamicDescriptorHeap::parseGraphicsRootSignature(const RootSignature& rootSig)
{
	graphicsHandleCache_.parseRootSignature(rootSig);
}

void DynamicDescriptorHeap::parseComputeRootSignature(const RootSignature& rootSig)
{
	computeHandleCache_.parseRootSignature(rootSig);
}


bool DynamicDescriptorHeap::hasSpace(uint32_t count) const
{
	return (pCurrentHeapPtr_ != nullptr && currentOffset_ + count <= DescriptorAllocatorPool::NUM_DESCRIPTORS_PER_HEAP);
}

void DynamicDescriptorHeap::retireCurrentHeap(void)
{
	// Don't retire unused heaps.
	if (currentOffset_ == 0)
	{
		X_ASSERT(pCurrentHeapPtr_ == nullptr, "Current heap pointer should not be null")(pCurrentHeapPtr_);
		return;
	}

	X_ASSERT_NOT_NULL(pCurrentHeapPtr_);
	retiredHeaps_.push_back(pCurrentHeapPtr_);
	pCurrentHeapPtr_ = nullptr;
	currentOffset_ = 0;
}

void DynamicDescriptorHeap::retireUsedHeaps(uint64_t fenceValue)
{
	pool_.discardDescriptorHeaps(fenceValue, retiredHeaps_);
	retiredHeaps_.clear();
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::getHeapPointer(void)
{
	if (pCurrentHeapPtr_ == nullptr)
	{
		X_ASSERT(currentOffset_ == 0, "")(currentOffset_);
		pCurrentHeapPtr_ = pool_.requestDescriptorHeap();

		firstDescriptor_ = DescriptorHandle(
			pCurrentHeapPtr_->GetCPUDescriptorHandleForHeapStart(),
			pCurrentHeapPtr_->GetGPUDescriptorHandleForHeapStart());
	}

	return pCurrentHeapPtr_;
}

DescriptorHandle DynamicDescriptorHeap::allocate(uint32_t count)
{
	DescriptorHandle ret = firstDescriptor_ + currentOffset_ * getDescriptorSize();
	currentOffset_ += count;
	return ret;
}


void DynamicDescriptorHeap::copyAndBindStagedTables(DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* pCmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*pSetFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE))
{

}

// Mark all descriptors in the cache as stale and in need of re-uploading.
void DynamicDescriptorHeap::unbindAllValid(void)
{
	graphicsHandleCache_.unbindAllValid();
	computeHandleCache_.unbindAllValid();
}

uint32_t DynamicDescriptorHeap::getDescriptorSize()
{
	if (descriptorSize_ == 0) {
		descriptorSize_ = pDevice_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	return descriptorSize_;
}

X_NAMESPACE_END