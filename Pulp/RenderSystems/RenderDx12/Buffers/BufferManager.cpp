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

BufferManager::BufferManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
    ContextManager& contextMan, DescriptorAllocator& descriptorAllocator) :
    pDevice_(pDevice),
    contextMan_(contextMan),
    descriptorAllocator_(descriptorAllocator),

    heap_(
        core::bitUtil::RoundUpToMultiple<size_t>(POOL_SIZE * POOL_ALLOCATION_SIZE,
            core::VirtualMem::GetPageSize())),
    pool_(
        heap_.start(),
        heap_.end(),
        PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE),
        PoolArena::getMemoryAlignmentRequirement(POOL_ALLOCATION_ALIGN),
        PoolArena::getMemoryOffsetRequirement()),
    arena_(&pool_, "VidMemHandlePool")
{
    arena->addChildArena(&arena_);
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
    // so I currently don't keep track of buffers allocated.
    // so only when stats are enalbed do i know if we have leaks.
#if VID_MEMORY_STATS

    if (stats_.numIndexBuffers > 0) {
        X_WARNING("VidMem", "%" PRIu32 " indexbuffer(s) are still active", stats_.numIndexBuffers);
    }

    if (stats_.numVertexBuffers > 0) {
        X_WARNING("VidMem", "%" PRIu32 " vertexbuffer(s) are still active", stats_.numVertexBuffers);
    }

    if (stats_.numConstBuffers > 0) {
        X_WARNING("VidMem", "%" PRIu32 " constbuffer(s) are still active", stats_.numConstBuffers);
    }

#endif // !VID_MEMORY_STATS
}

BufferManager::VertexBufferHandle BufferManager::createVertexBuf(uint32_t numElements, uint32_t elementSize, const void* pInitialData,
    BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
    X_ASSERT(numElements > 0, "NumElements size must be none zero")(numElements);
    X_ASSERT(elementSize > 0, "Element size must be none zero")(elementSize);

    core::CriticalSection::ScopedLock lock(cs_); // TODO: perf

    const uint32_t size = numElements * elementSize;

    X3DBuffer* pBuf = Int_CreateVB(size);

    pBuf->usage_ = usage;
    pBuf->offset_ = 0;
    pBuf->sizeBytes_ = size; // ??
    pBuf->size_ = size;
    pBuf->unPaddedSize_ = size;

    pBuf->pBuffer_ = X_NEW(ByteAddressBuffer, &arena_, "VbBuf");
    pBuf->pBackingHeap_ = nullptr;

    pBuf->pBuffer_->create(pDevice_, contextMan_, descriptorAllocator_, numElements, elementSize, pInitialData);

    D3DDebug::SetDebugObjectName(pBuf->pBuffer_->getResource(), "VertexBuffer");

    return createHandleForBuffer(pBuf);
}

BufferManager::IndexBufferHandle BufferManager::createIndexBuf(uint32_t numElements, uint32_t elementSize, const void* pInitialData,
    BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
    core::CriticalSection::ScopedLock lock(cs_);

    const uint32_t size = numElements * elementSize;

    X3DBuffer* pBuf = Int_CreateIB(size);

    pBuf->usage_ = usage;
    pBuf->offset_ = 0;
    pBuf->sizeBytes_ = size; // ??
    pBuf->size_ = size;
    pBuf->unPaddedSize_ = size;

    pBuf->pBuffer_ = X_NEW(ByteAddressBuffer, &arena_, "IbBuf");
    pBuf->pBackingHeap_ = nullptr;

    pBuf->pBuffer_->create(pDevice_, contextMan_, descriptorAllocator_, numElements, elementSize, pInitialData);

    D3DDebug::SetDebugObjectName(pBuf->pBuffer_->getResource(), "IndexBuffer");

    return createHandleForBuffer(pBuf);
}

BufferManager::ConstantBufferHandle BufferManager::createConstBuf(uint32_t size, const void* pInitialData,
    BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
    core::CriticalSection::ScopedLock lock(cs_);

    ConstBuffer* pBuf = Int_CreateCB(size);

    pBuf->usage_ = usage;
    pBuf->offset_ = 0;
    pBuf->sizeBytes_ = size; // ??
    pBuf->size_ = size;
    pBuf->unPaddedSize_ = size;

    pBuf->pBuffer_ = X_NEW(ByteAddressBuffer, &arena_, "CbBuf");
    pBuf->pBackingHeap_ = nullptr;

    pBuf->pBuffer_->create(pDevice_, contextMan_, descriptorAllocator_, size, 1, pInitialData);

    D3DDebug::SetDebugObjectName(pBuf->pBuffer_->getResource(), "ConstBuffer");

    return createHandleForBuffer(pBuf);
}

void BufferManager::freeIB(IndexBufferHandle IBHandle)
{
    core::CriticalSection::ScopedLock lock(cs_);

    X3DBuffer* pBuf = bufferForHandle(IBHandle);

#if VID_MEMORY_STATS
    stats_.indexesBytes -= 0; // TODO
    stats_.numIndexBuffers--;
#endif // !VID_MEMORY_STATS

    X_DELETE(pBuf->pBuffer_, &arena_);
    X_DELETE(pBuf, &arena_);
}

void BufferManager::freeVB(VertexBufferHandle VBHandle)
{
    core::CriticalSection::ScopedLock lock(cs_);

    X3DBuffer* pBuf = bufferForHandle(VBHandle);

#if VID_MEMORY_STATS
    stats_.vertexBytes -= 0; // TODO
    stats_.numVertexBuffers--;
#endif // !VID_MEMORY_STATS

    X_DELETE(pBuf->pBuffer_, &arena_);
    X_DELETE(pBuf, &arena_);
}

void BufferManager::freeCB(VertexBufferHandle CBHandle)
{
    core::CriticalSection::ScopedLock lock(cs_);

    X3DBuffer* pBuf = bufferForHandle(CBHandle);

#if VID_MEMORY_STATS
    stats_.constBufferBytes -= 0; // TODO
    stats_.numConstBuffers--;
#endif // !VID_MEMORY_STATS

    X_DELETE(pBuf->pBuffer_, &arena_);
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

ConstBuffer* BufferManager::Int_CreateCB(uint32_t size)
{
    ConstBuffer* pBuf = X_NEW(ConstBuffer, &arena_, "VidMemCB");

#if VID_MEMORY_STATS
    stats_.constBufferBytes += size;
    stats_.maxConstBufferBytes = core::Max(stats_.maxConstBufferBytes, stats_.constBufferBytes);
    stats_.numConstBuffers++;
    stats_.maxConstBuffers = core::Max(stats_.maxConstBuffers, stats_.numConstBuffers);
#endif // !VID_MEMORY_STATS

    return pBuf;
}

X_NAMESPACE_END
