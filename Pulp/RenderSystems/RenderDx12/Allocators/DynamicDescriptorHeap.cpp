#include "stdafx.h"
#include "DynamicDescriptorHeap.h"

#include "CommandListManger.h"
#include "CommandContex.h"
#include "RootSignature.h"

X_NAMESPACE_BEGIN(render)

const uint32_t DescriptorTypeAllocatorPool::NUM_DESCRIPTORS_PER_HEAP[DescriptorHeapType::ENUM_COUNT] = {
    1024, // CBV_SRV_UAV
    512   // SAMPLER
};

DescriptorTypeAllocatorPool::DescriptorTypeAllocatorPool(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
    CommandListManger& commandManager, D3D12_DESCRIPTOR_HEAP_TYPE type) :
    type_(type),
    pDevice_(pDevice),
    commandManager_(commandManager),
    retiredDescriptorHeaps_(arena, 128),
    availableDescriptorHeaps_(arena, 128),
    descriptorHeapPool_(arena)
{
}

DescriptorTypeAllocatorPool::~DescriptorTypeAllocatorPool()
{
}

void DescriptorTypeAllocatorPool::destoryAll(void)
{
    core::CriticalSection::ScopedLock lock(cs_);

    for (auto dh : descriptorHeapPool_) {
        core::SafeReleaseDX(dh);
    }

    descriptorHeapPool_.clear();
}

ID3D12DescriptorHeap* DescriptorTypeAllocatorPool::requestDescriptorHeap(void)
{
    core::CriticalSection::ScopedLock lock(cs_);

    // move and completed heaps into the available list.
    while (retiredDescriptorHeaps_.isNotEmpty() && commandManager_.isFenceComplete(retiredDescriptorHeaps_.peek().first)) {
        availableDescriptorHeaps_.push(retiredDescriptorHeaps_.peek().second);
        retiredDescriptorHeaps_.pop();
    }

    // any we can reuse?
    if (availableDescriptorHeaps_.isNotEmpty()) {
        ID3D12DescriptorHeap* pHeapPtr = availableDescriptorHeaps_.peek();
        availableDescriptorHeaps_.pop();
        return pHeapPtr;
    }
    else {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
        core::zero_object(heapDesc);
        heapDesc.Type = type_;
        heapDesc.NumDescriptors = NUM_DESCRIPTORS_PER_HEAP[type_];
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

void DescriptorTypeAllocatorPool::discardDescriptorHeaps(uint64_t fenceValue, const core::Array<ID3D12DescriptorHeap*>& usedHeaps)
{
    core::CriticalSection::ScopedLock lock(cs_);

    for (auto iter = usedHeaps.begin(); iter != usedHeaps.end(); ++iter) {
        X_ASSERT(retiredDescriptorHeaps_.size() < retiredDescriptorHeaps_.capacity(), "fifo is full")();
        retiredDescriptorHeaps_.push(std::make_pair(fenceValue, *iter));
    }
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorTypeAllocatorPool::getType(void) const
{
    return type_;
}

// ---------------------------------------------------------------------

DescriptorAllocatorPool::DescriptorAllocatorPool(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
    CommandListManger& commandManager) :
    allocators_{{{arena, pDevice, commandManager, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        {arena, pDevice, commandManager, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER}}}
{
    static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == 0, "Heap enum value has changed");
    static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == 1, "Heap enum value has changed");

    // do a runtime check also.
    X_ASSERT(allocators_[DescriptorHeapType::CBV_SRV_UAV].getType() == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, "Index error")();
    X_ASSERT(allocators_[DescriptorHeapType::SAMPLER].getType() == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, "Index error")();
}

void DescriptorAllocatorPool::destoryAll(void)
{
    for (auto& a : allocators_) {
        a.destoryAll();
    }
}

ID3D12DescriptorHeap* DescriptorAllocatorPool::requestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return allocators_[type].requestDescriptorHeap();
}

void DescriptorAllocatorPool::discardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE type, uint64_t fenceValueForReset,
    const core::Array<ID3D12DescriptorHeap*>& usedHeaps)
{
    allocators_[type].discardDescriptorHeaps(fenceValueForReset, usedHeaps);
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

    while ((rootIndex = core::bitUtil::ScanBitsForward(staleParams)) != core::bitUtil::NO_BIT_SET) {
        staleParams ^= (1 << rootIndex);

        const uint32_t maxSetHandle = core::bitUtil::ScanBits(rootDescriptorTable_[rootIndex].assignedHandlesBitMap);
        X_ASSERT(maxSetHandle != core::bitUtil::NO_BIT_SET, "Root entry masked as stable but has no stable descriptors")(maxSetHandle);

        neededSpace += maxSetHandle + 1;
    }

    return neededSpace;
}

void DynamicDescriptorHeap::DescriptorHandleCache::copyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12Device* pDevice,
    DescriptorHandle DestHandleStart, uint32_t descriptorSize,
    ID3D12GraphicsCommandList* pCmdList, SetRootDescriptorfunctionPtr pSetFunc)
{
    uint32_t staleParamCount = 0;
    uint32_t tableSize[DescriptorHandleCache::MAXNUM_DESCRIPTOR_TABLES];
    uint32_t rootIndices[DescriptorHandleCache::MAXNUM_DESCRIPTOR_TABLES];
    uint32_t neededSpace = 0;
    uint32_t rootIndex;

#if X_DEBUG
    core::fill_object(tableSize, 0xFF);
    core::fill_object(rootIndices, 0xFF);
#endif // !X_DEBUG

    // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
    uint32_t staleParams = staleRootParamsBitMap_;
    while ((rootIndex = core::bitUtil::ScanBitsForward(staleParams)) != core::bitUtil::NO_BIT_SET) {
        rootIndices[staleParamCount] = rootIndex;
        staleParams ^= (1 << rootIndex);

        const uint32_t maxSetHandle = core::bitUtil::ScanBits(rootDescriptorTable_[rootIndex].assignedHandlesBitMap);
        X_ASSERT(maxSetHandle != core::bitUtil::NO_BIT_SET, "Root entry masked as stable but has no stable descriptors")(maxSetHandle);

        neededSpace += maxSetHandle + 1;
        tableSize[staleParamCount] = maxSetHandle + 1;

        ++staleParamCount;
    }

    X_ASSERT(staleParamCount <= DescriptorHandleCache::MAXNUM_DESCRIPTOR_TABLES,
        "We're only equipped to handle so many descriptor tables")
    (staleParamCount);
    // you should not be calling this function if nothing is stale!
    X_ASSERT(staleParamCount > 0, "No params are stale")(staleParamCount);

    staleRootParamsBitMap_ = 0;

    uint32_t numDestDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[DescriptorHandleCache::MAX_DESCRIPTORS_PER_COPY];
    uint32_t pDestDescriptorRangeSizes[DescriptorHandleCache::MAX_DESCRIPTORS_PER_COPY];

    uint32_t numSrcDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[DescriptorHandleCache::MAX_DESCRIPTORS_PER_COPY];
    uint32_t pSrcDescriptorRangeSizes[DescriptorHandleCache::MAX_DESCRIPTORS_PER_COPY];

#if X_DEBUG
    // is this a good idea, might causes a bug to apepar only in release mode :S
    // instead of zero i'll set to -1
    core::fill_object(pDestDescriptorRangeSizes, 0xFF);
    core::fill_object(pSrcDescriptorRangeSizes, 0xFF);
#endif // !X_DEBUG

    for (uint32_t i = 0; i < staleParamCount; ++i) {
        rootIndex = rootIndices[i];
        (pCmdList->*pSetFunc)(rootIndex, DestHandleStart.getGpuHandle());

        const DescriptorTableCache& rootDescTable = rootDescriptorTable_[rootIndex];

        D3D12_CPU_DESCRIPTOR_HANDLE* pSrcHandles = rootDescTable.pTableStart;
        uint64_t setHandles = static_cast<uint64_t>(rootDescTable.assignedHandlesBitMap);
        D3D12_CPU_DESCRIPTOR_HANDLE curDest = DestHandleStart.getCpuHandle();
        DestHandleStart += tableSize[i] * descriptorSize;

        uint32_t skipCount;
        while ((skipCount = core::bitUtil::ScanBitsForward(setHandles)) != core::bitUtil::NO_BIT_SET) {
            // Skip over unset descriptor handles
            setHandles >>= skipCount;
            pSrcHandles += skipCount;
            curDest.ptr += skipCount * descriptorSize;

            const uint32_t descriptorCount = core::bitUtil::ScanBitsForward(~setHandles);
            X_ASSERT(descriptorCount != core::bitUtil::NO_BIT_SET, "bit not set")(descriptorCount);
            setHandles >>= descriptorCount;

            // If we run out of temp room, copy what we've got so far
            if (numSrcDescriptorRanges + descriptorCount > DescriptorHandleCache::MAX_DESCRIPTORS_PER_COPY) {
                // thread free
                pDevice->CopyDescriptors(
                    numDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                    numSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                    type);

                numSrcDescriptorRanges = 0;
                numDestDescriptorRanges = 0;
            }

            // Setup destination range
            pDestDescriptorRangeStarts[numDestDescriptorRanges] = curDest;
            pDestDescriptorRangeSizes[numDestDescriptorRanges] = descriptorCount;
            ++numDestDescriptorRanges;

            // Setup source ranges (one descriptor each because we don't assume they are contiguous)
            for (uint32_t j = 0; j < descriptorCount; ++j) {
                pSrcDescriptorRangeStarts[numSrcDescriptorRanges] = pSrcHandles[j];
                pSrcDescriptorRangeSizes[numSrcDescriptorRanges] = 1;
                ++numSrcDescriptorRanges;
            }

            // Move the destination pointer forward by the number of descriptors we will copy
            pSrcHandles += descriptorCount;
            curDest.ptr += descriptorCount * descriptorSize;
        }
    }

    // thread free
    pDevice->CopyDescriptors(
        numDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
        numSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
        type);
}

void DynamicDescriptorHeap::DescriptorHandleCache::unbindAllValid(void)
{
    staleRootParamsBitMap_ = 0;

    uint32_t tableParams = rootDescriptorTablesBitMap_;
    uint32_t rootIndex;

    while ((rootIndex = core::bitUtil::ScanBitsForward(tableParams)) != core::bitUtil::NO_BIT_SET) {
        tableParams ^= (1 << rootIndex);

        if (rootDescriptorTable_[rootIndex].assignedHandlesBitMap != 0) {
            staleRootParamsBitMap_ |= (1 << rootIndex);
        }
    }
}

void DynamicDescriptorHeap::DescriptorHandleCache::stageDescriptorHandles(uint32_t rootIndex, uint32_t offset,
    uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
    X_ASSERT(((1 << rootIndex) & rootDescriptorTablesBitMap_) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table")(rootIndex);
    // you basically called me with a invalid range for this rootIndex.
    X_ASSERT(offset + numHandles <= rootDescriptorTable_[rootIndex].tableSize, "Cache overrun for root index: %" PRIi32, rootIndex)(rootIndex);

    DescriptorTableCache& tableCache = rootDescriptorTable_[rootIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE* pCopyDest = tableCache.pTableStart + offset;
    for (uint32_t i = 0; i < numHandles; ++i) {
        pCopyDest[i] = pHandles[i];
    }

    tableCache.assignedHandlesBitMap |= ((1 << numHandles) - 1) << offset;
    staleRootParamsBitMap_ |= (1 << rootIndex);
}

void DynamicDescriptorHeap::DescriptorHandleCache::parseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig)
{
    uint32_t CurrentOffset = 0;

    X_ASSERT(rootSig.numParams() <= MAXNUM_DESCRIPTOR_TABLES, "Maybe we need to support something greater")(rootSig.numParams());

    staleRootParamsBitMap_ = 0;
    rootDescriptorTablesBitMap_ = rootSig.descriptorTableBitMap(type);

    uint32_t tableParams = rootDescriptorTablesBitMap_;
    uint32_t rootIndex;
    while ((rootIndex = core::bitUtil::ScanBitsForward(tableParams)) != core::bitUtil::NO_BIT_SET) {
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

    X_ASSERT(maxCachedDescriptors_ <= MAXNUM_DESCRIPTORS, "Exceeded user-supplied maximum cache size")(maxCachedDescriptors_, MAXNUM_DESCRIPTORS);
}

// ---------------------------------------------------------------------

DynamicDescriptorHeap::DynamicDescriptorHeap(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
    DescriptorAllocatorPool& pool, CommandContext& owningContext, D3D12_DESCRIPTOR_HEAP_TYPE type) :
    type_(type),
    pDevice_(pDevice),
    retiredHeaps_(arena),
    owningContext_(owningContext),
    pool_(pool),
    pCurrentHeapPtr_(nullptr),
    currentOffset_(0),
    descriptorSize_(0)
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

    owningContext_.setDescriptorHeap(type_, getHeapPointer());

    DescriptorHandle destHandle = firstDescriptor_ + currentOffset_ * getDescriptorSize();
    currentOffset_ += 1;

    // thread free
    pDevice_->CopyDescriptorsSimple(1, destHandle.getCpuHandle(), handle, type_);

    return destHandle.getGpuHandle();
}

void DynamicDescriptorHeap::parseGraphicsRootSignature(const RootSignature& rootSig)
{
    graphicsHandleCache_.parseRootSignature(type_, rootSig);
}

void DynamicDescriptorHeap::parseComputeRootSignature(const RootSignature& rootSig)
{
    computeHandleCache_.parseRootSignature(type_, rootSig);
}

bool DynamicDescriptorHeap::hasSpace(uint32_t count) const
{
    return (pCurrentHeapPtr_ != nullptr && currentOffset_ + count <= DescriptorTypeAllocatorPool::NUM_DESCRIPTORS_PER_HEAP[type_]);
}

void DynamicDescriptorHeap::retireCurrentHeap(void)
{
    // Don't retire unused heaps.
    if (currentOffset_ == 0) {
        // we should not of allocated a heap if we never used it.
        // currently this will leak.
        X_ASSERT(pCurrentHeapPtr_ == nullptr, "Heap was allocated but never used")(pCurrentHeapPtr_);
        return;
    }

    X_ASSERT_NOT_NULL(pCurrentHeapPtr_);
    retiredHeaps_.push_back(pCurrentHeapPtr_);
    pCurrentHeapPtr_ = nullptr;
    currentOffset_ = 0;
}

void DynamicDescriptorHeap::retireUsedHeaps(uint64_t fenceValue)
{
    pool_.discardDescriptorHeaps(type_, fenceValue, retiredHeaps_);
    retiredHeaps_.clear();
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::getHeapPointer(void)
{
    if (pCurrentHeapPtr_ == nullptr) {
        X_ASSERT(currentOffset_ == 0, "Current heap should of been retired before requesting a new one")(currentOffset_);
        pCurrentHeapPtr_ = pool_.requestDescriptorHeap(type_);

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
    SetRootDescriptorfunctionPtr pSetFunc)
{
    uint32_t neededSize = handleCache.computeStagedSize();

    if (!hasSpace(neededSize)) {
        retireCurrentHeap();
        unbindAllValid();

        // recalculate the size it might be diffrent if stale params has changed.
        neededSize = handleCache.computeStagedSize();

#if X_DEBUG
        getHeapPointer();
        if (!hasSpace(neededSize)) {
            // not even a empty heap can fit the request :(
            X_ASSERT_UNREACHABLE();
            return;
        }
#endif // !X_DEBUG
    }

    // This can trigger the creation of a new heap
    owningContext_.setDescriptorHeap(type_, getHeapPointer());

    handleCache.copyAndBindStaleTables(type_, pDevice_, allocate(neededSize), getDescriptorSize(), pCmdList, pSetFunc);
}

// Mark all descriptors in the cache as stale and in need of re-uploading.
void DynamicDescriptorHeap::unbindAllValid(void)
{
    graphicsHandleCache_.unbindAllValid();
    computeHandleCache_.unbindAllValid();
}

uint32_t DynamicDescriptorHeap::getDescriptorSize(void)
{
    if (descriptorSize_ == 0) {
        descriptorSize_ = pDevice_->GetDescriptorHandleIncrementSize(type_);
    }
    return descriptorSize_;
}

X_NAMESPACE_END