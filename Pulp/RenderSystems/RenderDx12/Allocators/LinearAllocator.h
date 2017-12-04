#pragma once

#include "GpuResource.h"

#include <Containers\Fifo.h>
#include <Containers\PriorityQueue.h>

class CommandListManger;

X_NAMESPACE_BEGIN(render)

/*

LinearAllocator:
UseCase: Lots of small allocations per frame.
Cleanup: Automatic

Details:
	The linera allocator allocates 'pages' of memory and sub allocates the block
	returning DynAlloc.

	You can't make a single allocation bigger than size of a single page.
	Multiple pages will be made per frame if needed.

	All the pages for a frame are automatically cleaned up once the required fence value has been reached
	to signal the buffers are no longer in use.

*/

X_DECLARE_ENUM(LinearAllocatorType)(
	GPU_EXCLUSIVE,
	CPU_WRITABLE
);

// DynAlloc holds infomation about a allocation.
// Various types of allocations may contain NULL pointers.
// Check before dereferencing if you are unsure.
struct DynAlloc
{
	DynAlloc(GpuResource& baseResource, size_t offset, size_t size);

	X_INLINE void setData(void* pData, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);


	X_INLINE size_t getOffset(void) const;
	X_INLINE size_t getSize(void) const;
	X_INLINE void* getCpuData(void) const;
	X_INLINE const GpuResource& getBuffer(void) const;
	X_INLINE GpuResource& getBuffer(void);
	X_INLINE D3D12_GPU_VIRTUAL_ADDRESS getGpuAddress(void);

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

private:
	void* pCpuVirtualAddress_;
};


class LinearAllocatorPageManager
{
public:
	static const uint64_t GPU_ALLOCATOION_PAGE_SIZE = 1024 * 64; // 64K
	static const uint64_t CPU_ALLOCATOION_PAGE_SIZE = 1024 * 1024 * 2; // 2MB

	typedef core::Array<LinearAllocationPage*> LineraAllocationPageArr;
	typedef core::PriorityQueue<std::pair<uint64_t, LinearAllocationPage*> > AllocationFencePageQueue;
	typedef core::Fifo<LinearAllocationPage*> AllocationPageQueue;

public:
	LinearAllocatorPageManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		CommandListManger& cmdMan, LinearAllocatorType::Enum type);
	~LinearAllocatorPageManager();

	LinearAllocationPage* requestPage(void);
	void discardPages(uint64_t fenceID, const LineraAllocationPageArr& pages);
	void destroy(void);

private:
	LinearAllocationPage* createNewPage(void);

private:
	core::MemoryArenaBase* arena_;
	ID3D12Device* pDevice_;
	CommandListManger& cmdMan_;
	core::CriticalSection cs_;

	LinearAllocatorType::Enum allocationType_;
	LineraAllocationPageArr pagePool_;
	AllocationFencePageQueue retiredPages_;
	AllocationPageQueue availablePages_;
};

class LinearAllocatorManager
{
	typedef std::array<LinearAllocatorPageManager, LinearAllocatorType::ENUM_COUNT> AllocationPageManagerArr;
	typedef core::Array<LinearAllocationPage*> LineraAllocationPageArr;

public:
	LinearAllocatorManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		CommandListManger& cmdMan);

	X_INLINE LinearAllocationPage* requestPage(LinearAllocatorType::Enum type);
	X_INLINE void discardPages(LinearAllocatorType::Enum type, uint64_t fenceID, const LineraAllocationPageArr& pages);
	void destroy(void);

private:
	AllocationPageManagerArr pageAllocators_;
};

class LinearAllocator
{
public:
	static const size_t DEFAULT_ALIGN = 256;

	typedef core::Array<LinearAllocationPage*> LineraAllocationPageArr;

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
	LineraAllocationPageArr retiredPages_;
};


X_NAMESPACE_END

#include "LinearAllocator.inl"