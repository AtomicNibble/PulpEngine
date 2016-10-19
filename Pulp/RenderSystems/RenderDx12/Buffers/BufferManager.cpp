#include "stdafx.h"
#include "BufferManager.h"

#include <Memory\VirtualMem.h>

X_NAMESPACE_BEGIN(render)



X3DBuffer::X3DBuffer() : 
	sizeBytes_(0),
	offset_(0),
	size_(0),
	unPaddedSize_(0),
	pBuffer_(nullptr),
	pBackingHeap_(nullptr)
{

}

X3DBuffer::~X3DBuffer()
{

}

// -------------------------------------------

BufferManager::Stats::Stats()
{
	core::zero_this(this);
}

// -------------------------------------------

BufferManager::BufferManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
	pDevice_(pDevice),

	heap_(
		core::bitUtil::RoundUpToMultiple<size_t>(POOL_SIZE * POOL_ALLOCATION_SIZE,
			core::VirtualMem::GetPageSize())
	),
	pool_(heap_.start(), heap_.end(), POOL_ALLOCATION_SIZE, POOL_ALLOCATION_ALIGN, 0),
	arena_(&pool_, "VidMemBuffer")
{
	X_UNUSED(arena);
}

BufferManager::~BufferManager()
{

}


bool BufferManager::init(void)
{

	return true;
}

void BufferManager::shutDown(void)
{

}

BufferManager::VertexBufferHandle BufferManager::createVertexBuf(uint32_t size, const void* pInitialData,
	BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X3DBuffer* pBuf = Int_CreateVB(size);


	return createHandleForBuffer(pBuf);
}


BufferManager::IndexBufferHandle BufferManager::createIndexBuf(uint32_t size, const void* pInitialData,
	BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	X3DBuffer* pBuf = Int_CreateVB(size);


	return createHandleForBuffer(pBuf);
}

void BufferManager::freeIB(IndexBufferHandle IBHandle)
{
	X3DBuffer* pBuf = bufferForHandle(IBHandle);

#if VID_MEMORY_STATS
	stats_.indexesBytes -= 0; // TODO
	stats_.numIndexBuffers--;
#endif // !VID_MEMORY_STATS

	X_DELETE(pBuf, &arena_);
}

void BufferManager::freeVB(VertexBufferHandle VBHandle)
{
	X3DBuffer* pBuf = bufferForHandle(VBHandle);

#if VID_MEMORY_STATS
	stats_.vertexBytes -= 0; // TODO
	stats_.numVertexBuffers--;
#endif // !VID_MEMORY_STATS
	
	X_DELETE(pBuf, &arena_);
}

void BufferManager::getBufSize(BufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize) const
{
	X3DBuffer* pBuf = bufferForHandle(handle);
	X_ASSERT_NOT_NULL(pBuf);
	X_ASSERT_NOT_NULL(pOriginal);

	*pOriginal = pBuf->getSize();

	// for now just the same.
	if (pDeviceSize) {
		*pDeviceSize = pBuf->getSize();
	}
}



BufferManager::Stats BufferManager::getStats(void) const
{
#if VID_MEMORY_STATS
	return stats_;
#else
	static Stats stats;
	return stats;
#endif // !VID_MEMORY_STATS
}


X3DBuffer* BufferManager::Int_CreateVB(uint32_t size)
{
	X3DBuffer* pBuf = X_NEW(X3DBuffer, &arena_, "VidMemVertex");

#if VID_MEMORY_STATS
	stats_.vertexBytes += size;
	stats_.maxVertexBytes = core::Max(stats_.maxVertexBytes, stats_.vertexBytes);
	stats_.numVertexBuffers++;
	stats_.maxVertexBuffers = core::Max(stats_.maxVertexBuffers, stats_.numVertexBuffers);
#endif // !VID_MEMORY_STATS

	return pBuf;
}

X3DBuffer* BufferManager::Int_CreateIB(uint32_t size)
{
	X3DBuffer* pBuf = X_NEW(X3DBuffer, &arena_, "VidMemIndexes");

#if VID_MEMORY_STATS
	stats_.indexesBytes += size;
	stats_.maxIndexesBytes = core::Max(stats_.maxIndexesBytes, stats_.indexesBytes);
	stats_.numIndexBuffers++;
	stats_.maxIndexBuffers = core::Max(stats_.maxIndexBuffers, stats_.numIndexBuffers);
#endif // !VID_MEMORY_STATS

	return pBuf;
}

BufferManager::BufferHandle BufferManager::createHandleForBuffer(X3DBuffer* pBuf)
{
	return reinterpret_cast<BufferManager::BufferHandle>(pBuf);
}

X3DBuffer* BufferManager::bufferForHandle(BufferHandle handle) const
{
	return reinterpret_cast<X3DBuffer*>(handle);
}



X_NAMESPACE_END

