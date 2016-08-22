#pragma once

#include <Containers\Array.h>
#include <Containers\Fifo.h>

#include <Memory\HeapArea.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\ThreadPolicies\SingleThreadPolicy.h>
#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\MemoryTaggingPolicies\SimpleMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\SimpleMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryArena.h>

#include "Buffers\GpuBuffer.h"

X_NAMESPACE_BEGIN(render)

// all like vertex and index buffers we give back to 3dengine should come from here
// then the 3dengine can just pass back the handle and we understand it.
// i might change if from a handle to a interface.
// so the the 3d engine has a way to update stuff.
// or maybe it's better if the 2d engine has to do it via the render system.
// then i might be able to track changes a bit better to resources.

class X3DBuffer
{
public:
	X3DBuffer();
	~X3DBuffer();

	X_INLINE const ByteAddressBuffer& getBuf(void) const;

private:

	uint32_t sizeBytes_;
	uint32_t offset_;
	uint32_t size_;
	uint32_t unPaddedSize_;

	ByteAddressBuffer* pBuffer_;
	ID3D12Heap* pBackingHeap_;
};

class BufferManager
{
public:
	static const size_t MAX_BUFFERS = 4096;

	typedef Commands::VertexBufferHandle VertexBufferHandle;
	typedef Commands::IndexBufferHandle IndexBufferHandle;
	typedef Commands::IndexBufferHandle BufferHandle;

	typedef core::MemoryArena<core::PoolAllocator,
		core::SingleThreadPolicy,
		core::NoBoundsChecking,
#if X_DEBUG
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif !X_DEBUG
	> PoolArena;


	struct Stats
	{
		Stats();

		uint32_t numIndexBuffers;
		uint32_t numVertexBuffers;
		uint32_t maxIndexBuffers;
		uint32_t maxVertexBuffers;

		uint32_t indexesBytes;
		uint32_t vertexBytes;
		uint32_t maxIndexesBytes;
		uint32_t maxVertexBytes;
	};

public:
	BufferManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice);
	~BufferManager();

	VertexBufferHandle createVertexBuf(uint32_t size, const void* pInitialData, IRender::BufUsage::Enum usage, IRender::CpuAccessFlags accessFlag);
	IndexBufferHandle createIndexBuf(uint32_t size, const void* pInitialData, IRender::BufUsage::Enum usage, IRender::CpuAccessFlags accessFlag);

	// free from ID
	void freeIB(IndexBufferHandle IBHandle);
	void freeVB(VertexBufferHandle VBHandle);

	// get the buffer from a ID
	X3DBuffer* IBFromHandle(IndexBufferHandle bufHandle) const;
	X3DBuffer* VBFromHandle(VertexBufferHandle bufHandle) const;

	Stats getStats(void) const;

private:
	// Internal create
	X3DBuffer* Int_CreateVB(uint32_t size);
	X3DBuffer* Int_CreateIB(uint32_t size);

	BufferHandle createHandleForBuffer(X3DBuffer* pBuf);
	X3DBuffer* bufferForHandle(BufferHandle handle) const;

private:
	ID3D12Device* pDevice_;

	core::HeapArea heap_;
	core::PoolAllocator pool_;
	PoolArena arena_;

private:
#if VID_MEMORY_STATS
	Stats stats_;
#endif // !VID_MEMORY_STATS
};


X_NAMESPACE_END


#include "BufferManager.inl"