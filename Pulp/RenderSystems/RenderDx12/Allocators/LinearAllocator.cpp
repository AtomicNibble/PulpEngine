#include "stdafx.h"
#include "CommandListManger.h"

#include "LinearAllocator.h"

X_NAMESPACE_BEGIN(render)

DynAlloc::DynAlloc(GpuResource& baseResource, size_t offset, size_t size) :
    buffer_(baseResource),
    offset_(offset),
    size_(size),
    pData_(nullptr)
{
    gpuAddress_ = 0;
}

// --------------------------------------------------------------------

LinearAllocationPage::LinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES usage) :
    GpuResource()
{
    X_ASSERT_NOT_NULL(pResource);

    pResource_ = pResource;
    usageState_ = usage;
    gpuVirtualAddress_ = pResource->GetGPUVirtualAddress();

    HRESULT hr = pResource->Map(0, nullptr, &pCpuVirtualAddress_);
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to map resource. err: %" PRIu32, hr);
    }
}

LinearAllocationPage::~LinearAllocationPage()
{
    pResource_->Unmap(0, nullptr);
}

// --------------------------------------------------------------------

LinearAllocatorPageManager::LinearAllocatorPageManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
    CommandListManger& cmdMan, LinearAllocatorType::Enum type) :
    arena_(arena),
    pDevice_(pDevice),
    cmdMan_(cmdMan),
    allocationType_(type),
    pagePool_(arena),
    deletedPages_(arena),
    retiredPages_(arena),
    availablePages_(arena)
{
}

LinearAllocatorPageManager::~LinearAllocatorPageManager()
{
    destroy();
}

LinearAllocationPage* LinearAllocatorPageManager::requestPage(void)
{
    core::CriticalSection::ScopedLock lock(cs_);

    while (retiredPages_.isNotEmpty() && cmdMan_.isFenceComplete(retiredPages_.peek().first)) {
        availablePages_.push(retiredPages_.peek().second);
        retiredPages_.pop();
    }

    LinearAllocationPage* pPagePtr = nullptr;

    if (availablePages_.isNotEmpty()) {
        pPagePtr = availablePages_.front();
        availablePages_.pop();
    }
    else {
        pPagePtr = createNewPage();
        pagePool_.emplace_back(pPagePtr);
    }

    return pPagePtr;
}

LinearAllocationPage* LinearAllocatorPageManager::allocatePage(size_t sizeInBytes)
{
    core::CriticalSection::ScopedLock lock(cs_);

    return createNewPage(sizeInBytes);
}

void LinearAllocatorPageManager::discardPages(uint64_t fenceID, const LineraAllocationPageArr& pages)
{
    core::CriticalSection::ScopedLock lock(cs_);

    for (auto iter = pages.begin(); iter != pages.end(); ++iter) {
        retiredPages_.push(std::make_pair(fenceID, *iter));
    }
}

void LinearAllocatorPageManager::freePages(uint64_t fenceID, const LineraAllocationPageArr& pages)
{
    core::CriticalSection::ScopedLock lock(cs_);

    while (deletedPages_.isNotEmpty() && cmdMan_.isFenceComplete(deletedPages_.peek().first)) {
        X_DELETE(deletedPages_.peek().second, nullptr);
        deletedPages_.pop();
    }

    for (auto iter = pages.begin(); iter != pages.end(); ++iter) {
        // Unmap now?
        // (*iter);
        deletedPages_.push(std::make_pair(fenceID, *iter));
    }
}

void LinearAllocatorPageManager::destroy(void)
{
    for (LinearAllocationPage* p : pagePool_) {
        X_DELETE(p, arena_);
    }

    pagePool_.clear();
}

LinearAllocationPage* LinearAllocatorPageManager::createNewPage(size_t sizeInBytes)
{
    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC ResourceDesc;
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Alignment = 0;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_RESOURCE_STATES defaultUsage;

    if (allocationType_ == LinearAllocatorType::GPU_EXCLUSIVE) {
        HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        ResourceDesc.Width = sizeInBytes == 0 ? GPU_ALLOCATOION_PAGE_SIZE : sizeInBytes;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        defaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    else {
        HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        ResourceDesc.Width = sizeInBytes == 0 ? CPU_ALLOCATOION_PAGE_SIZE : sizeInBytes;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        defaultUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    static_assert(LinearAllocatorType::ENUM_COUNT == 2, "LinearAllocatorType enum count changed");

    ID3D12Resource* pBuffer;
    HRESULT hr = pDevice_->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
        defaultUsage, nullptr, IID_PPV_ARGS(&pBuffer));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create commited resource: err: %" PRIu32, hr);
    }

    D3DDebug::SetDebugObjectName(pBuffer, L"LinearAllocator Page");

    return X_NEW(LinearAllocationPage, arena_, "GpuLinAllocPage")(pBuffer, defaultUsage);
}

// --------------------------------------------------------------------

LinearAllocatorManager::LinearAllocatorManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
    CommandListManger& cmdMan) :
    pageAllocators_{{
        {arena, pDevice, cmdMan, LinearAllocatorType::GPU_EXCLUSIVE},
        {arena, pDevice, cmdMan, LinearAllocatorType::CPU_WRITABLE},
    }}
{
}

void LinearAllocatorManager::destroy(void)
{
    for (auto& lapm : pageAllocators_) {
        lapm.destroy();
    }
}

// --------------------------------------------------------------------

LinearAllocator::LinearAllocator(core::MemoryArenaBase* arena, LinearAllocatorManager& manager, LinearAllocatorType::Enum type) :
    manager_(manager),
    allocationType_(type),
    pageSize_(type == LinearAllocatorType::GPU_EXCLUSIVE ? LinearAllocatorPageManager::GPU_ALLOCATOION_PAGE_SIZE : LinearAllocatorPageManager::CPU_ALLOCATOION_PAGE_SIZE),
    curOffset_(0),
    pCurPage_(nullptr),
    retiredPages_(arena),
    largePages_(arena)
{
}

LinearAllocator::~LinearAllocator()
{
    X_ASSERT(pCurPage_ == nullptr, "Current page was not cleaned up")
    (pCurPage_);
    X_ASSERT(retiredPages_.isEmpty(), "Retired pages where not discarded")
    (retiredPages_.size());
    X_ASSERT(largePages_.isEmpty(), "Large pages where not discarded")
    (largePages_.size());
}

DynAlloc LinearAllocator::allocate(size_t sizeInBytes, size_t alignment)
{
    X_ASSERT(sizeInBytes <= pageSize_, "Exceeded max linear allocator page size with single allocation")
    (sizeInBytes, pageSize_);

    // Assert that it's a power of two.
    X_ASSERT(core::bitUtil::IsPowerOfTwo(alignment), "alignment must be power of two")
    (alignment);

    const size_t alignedSize = core::bitUtil::RoundUpToMultiple(sizeInBytes, alignment);

    if (alignedSize > pageSize_) {
        return allocateLarge(sizeInBytes);
    }

    curOffset_ = core::bitUtil::RoundUpToMultiple(curOffset_, alignment);

    if (curOffset_ + alignedSize > pageSize_) {
        X_ASSERT_NOT_NULL(pCurPage_);
        retiredPages_.push_back(pCurPage_);
        pCurPage_ = nullptr;
    }

    if (pCurPage_ == nullptr) {
        pCurPage_ = manager_.requestPage(allocationType_);
        curOffset_ = 0;

        X_ASSERT_NOT_NULL(pCurPage_);
    }

    void* pData = (reinterpret_cast<uint8_t*>(pCurPage_->cpuVirtualAddress()) + curOffset_);
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pCurPage_->getGpuVirtualAddress() + curOffset_;

    DynAlloc ret(*pCurPage_, curOffset_, alignedSize);
    ret.setData(pData, gpuAddress);

    curOffset_ += alignedSize;

    return ret;
}

DynAlloc LinearAllocator::allocateLarge(size_t sizeInBytes)
{
    LinearAllocationPage* pPage = manager_.allocatePage(allocationType_, sizeInBytes);
    largePages_.push_back(pPage);

    DynAlloc ret(*pPage, 0, sizeInBytes);
    ret.setData(pPage->cpuVirtualAddress(), pPage->getGpuVirtualAddress());

    return ret;
}

void LinearAllocator::cleanupUsedPages(uint64_t fenceID)
{
    if (!pCurPage_) {
        return;
    }

    retiredPages_.push_back(pCurPage_);
    pCurPage_ = nullptr;
    curOffset_ = 0;

    manager_.discardPages(allocationType_, fenceID, retiredPages_);
    manager_.freePages(allocationType_, fenceID, largePages_);

    retiredPages_.clear();
    largePages_.clear();
}

X_NAMESPACE_END