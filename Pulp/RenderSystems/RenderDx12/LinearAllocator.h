#pragma once

#include "GpuResource.h"
#include <queue>

class CommandListManger;

X_NAMESPACE_BEGIN(render)


X_DECLARE_ENUM(LinearAllocatorType)(
	GPU_EXCLUSIVE,
	CPU_WRITABLE
);


// Various types of allocations may contain NULL pointers.  Check before dereferencing if you are unsure.
struct DynAlloc
{
	DynAlloc(GpuResource& baseResource, size_t offset, size_t size);

	X_INLINE void setData(void* pData, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);

private:
	GpuResource& buffer_;	// The D3D buffer associated with this memory.
	size_t offset_;			// Offset from start of buffer resource
	size_t size_;			// Reserved size of this allocation
	void* pData_;			// The CPU-writeable address
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress_;	// The GPU-visible address
};

class LinearAllocationPage : public GpuResource
{
public:
	LinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES usage);
	~LinearAllocationPage();


	X_INLINE void* cpuVirtualAddress(void) const;
	X_INLINE D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress(void) const;

private:
	void* pCpuVirtualAddress_;
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress_;
};


class LinearAllocatorPageManager
{
public:
	static const uint64_t GPU_ALLOCATOION_PAGE_SIZE = 1024 * 64; // 64K
	static const uint64_t CPU_ALLOCATOION_PAGE_SIZE = 1024 * 1024 * 2; // 2MB

public:
	LinearAllocatorPageManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		CommandListManger& cmdMan, LinearAllocatorType::Enum type);
	~LinearAllocatorPageManager();

	LinearAllocationPage* requestPage(void);
	void discardPages(uint64_t fenceID, const core::Array<LinearAllocationPage*>& pages);
	void destroy(void);

private:
	LinearAllocationPage* createNewPage(void);

private:
	core::MemoryArenaBase* arena_;
	ID3D12Device* pDevice_;
	CommandListManger& cmdMan_;
	core::CriticalSection cs_;

	LinearAllocatorType::Enum allocationType_;
	core::Array<LinearAllocationPage*> pagePool_;
	std::queue<std::pair<uint64_t, LinearAllocationPage*> > retiredPages_;
	std::queue<LinearAllocationPage*> availablePages_;
};

class LinearAllocatorManager
{
public:
	LinearAllocatorManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		CommandListManger& cmdMan);

	X_INLINE LinearAllocationPage* requestPage(LinearAllocatorType::Enum type);
	X_INLINE void discardPages(LinearAllocatorType::Enum type, uint64_t fenceID, const core::Array<LinearAllocationPage*>& pages);

private:
	std::array<LinearAllocatorPageManager, LinearAllocatorType::ENUM_COUNT> pageAllocators_;
};

class LinearAllocator
{
public:
	static const size_t DEFAULT_ALIGN = 256;

public:
	LinearAllocator(core::MemoryArenaBase* arena, LinearAllocatorManager& manager, LinearAllocatorType::Enum Type);

	DynAlloc allocate(size_t sizeInBytes, size_t alignment = DEFAULT_ALIGN);
	void cleanupUsedPages(uint64_t fenceID);

private:
	LinearAllocatorManager& manager_;
	LinearAllocatorType::Enum allocationType_;
	size_t pageSize_;
	size_t curOffset_;
	LinearAllocationPage* pCurPage_;
	core::Array<LinearAllocationPage*> retiredPages_;
};


X_NAMESPACE_END

#include "LinearAllocator.inl"