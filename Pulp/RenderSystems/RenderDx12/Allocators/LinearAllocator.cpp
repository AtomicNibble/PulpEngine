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


LinearAllocationPage::LinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES usage) : GpuResource()
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
	pagePool_(arena)
{

}

LinearAllocatorPageManager::~LinearAllocatorPageManager()
{
	destroy();
}

LinearAllocationPage* LinearAllocatorPageManager::requestPage(void)
{
	core::CriticalSection::ScopedLock lock(cs_);

	while (!retiredPages_.empty() && cmdMan_.isFenceComplete(retiredPages_.front().first))
	{
		availablePages_.push(retiredPages_.front().second);
		retiredPages_.pop();
	}

	LinearAllocationPage* pPagePtr = nullptr;

	if (!availablePages_.empty())
	{
		pPagePtr = availablePages_.front();
		availablePages_.pop();
	}
	else
	{
		pPagePtr = createNewPage();
		pagePool_.emplace_back(pPagePtr);
	}

	return pPagePtr;
}

void LinearAllocatorPageManager::discardPages(uint64_t fenceID, const core::Array<LinearAllocationPage*>& pages)
{
	core::CriticalSection::ScopedLock lock(cs_);

	for (auto iter = pages.begin(); iter != pages.end(); ++iter) {
		retiredPages_.push(std::make_pair(fenceID, *iter));
	}
}

void LinearAllocatorPageManager::destroy(void)
{
	for (LinearAllocationPage* p : pagePool_) {
		X_DELETE(p, arena_);
	}

	pagePool_.clear();
}

LinearAllocationPage* LinearAllocatorPageManager::createNewPage(void)
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

	if (allocationType_ == LinearAllocatorType::GPU_EXCLUSIVE)
	{
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		ResourceDesc.Width = GPU_ALLOCATOION_PAGE_SIZE;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		defaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else
	{
		HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		ResourceDesc.Width = CPU_ALLOCATOION_PAGE_SIZE;
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

	D3DDebug::SetDebugObjectName(pBuffer,L"LinearAllocator Page");

	return X_NEW(LinearAllocationPage, arena_, "GpuLinAllocPage")(pBuffer, defaultUsage);
}

// --------------------------------------------------------------------

LinearAllocatorManager::LinearAllocatorManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
	CommandListManger& cmdMan) :
	pageAllocators_{{
		{ arena, pDevice, cmdMan, LinearAllocatorType::GPU_EXCLUSIVE },
		{ arena, pDevice, cmdMan, LinearAllocatorType::CPU_WRITABLE },
	}}
{

}


void LinearAllocatorManager::destroy(void)
{
	for (auto& lapm : pageAllocators_)
	{
		lapm.destroy();
	}
}

// --------------------------------------------------------------------


LinearAllocator::LinearAllocator(core::MemoryArenaBase* arena, LinearAllocatorManager& manager, LinearAllocatorType::Enum type) :
	manager_(manager),
	allocationType_(type),
	pageSize_(0),
	curOffset_(0),
	pCurPage_(nullptr),
	retiredPages_(arena)
{
	pageSize_ = LinearAllocatorPageManager::CPU_ALLOCATOION_PAGE_SIZE;
	if (type == LinearAllocatorType::GPU_EXCLUSIVE) {
		pageSize_ = LinearAllocatorPageManager::GPU_ALLOCATOION_PAGE_SIZE;
	}
}

DynAlloc LinearAllocator::allocate(size_t sizeInBytes, size_t alignment)
{
	X_ASSERT(sizeInBytes <= pageSize_, "Exceeded max linear allocator page size with single allocation")(sizeInBytes, pageSize_);

	// Assert that it's a power of two.
	X_ASSERT(core::bitUtil::IsPowerOfTwo(alignment), "alignment must be power of two")(alignment);

	const size_t alignedSize = core::bitUtil::RoundUpToMultiple(sizeInBytes, alignment);

	curOffset_ = core::bitUtil::RoundUpToMultiple(curOffset_, alignment);

	if (curOffset_ + alignedSize > pageSize_)
	{
		X_ASSERT_NOT_NULL(pCurPage_);
		retiredPages_.push_back(pCurPage_);
		pCurPage_ = nullptr;
	}

	if (pCurPage_ == nullptr)
	{
		pCurPage_ = manager_.requestPage(allocationType_);
		curOffset_ = 0;
	}

	void* pData = (reinterpret_cast<uint8_t*>(pCurPage_->cpuVirtualAddress()) + curOffset_);
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = pCurPage_->getGpuVirtualAddress() + curOffset_;

	DynAlloc ret(*pCurPage_, curOffset_, alignedSize);
	ret.setData(pData, gpuAddress);

	curOffset_ += alignedSize;

	return ret;
}

void LinearAllocator::cleanupUsedPages(uint64_t fenceID)
{
	if (pCurPage_) {
		return;
	}

	retiredPages_.push_back(pCurPage_);
	pCurPage_ = nullptr;
	curOffset_ = 0;

	manager_.discardPages(allocationType_, fenceID, retiredPages_);
	retiredPages_.clear();
}



X_NAMESPACE_END