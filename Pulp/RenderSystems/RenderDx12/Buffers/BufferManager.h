#pragma once

#include <Containers\Array.h>

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
	X_INLINE ByteAddressBuffer& getBuf(void);

	X_INLINE BufUsage::Enum getUsage(void) const;

	X_INLINE uint32_t getSize(void) const;

public:
	BufUsage::Enum usage_;

	uint32_t sizeBytes_;
	uint32_t offset_;
	uint32_t size_;
	uint32_t unPaddedSize_;

	// this will always be set, it might be placed if created from a backing heap.
	ByteAddressBuffer* pBuffer_;
	ID3D12Heap* pBackingHeap_;
};


struct ConstBuffer : X3DBuffer
{
	// ...
};


class BufferManager
{
public:
	static const size_t MIN_DEVICE_BUF_SIZE = 1024 * 64;
	static const size_t POOL_SIZE = 8192;
	static const size_t POOL_ALLOCATION_SIZE = core::Max(core::Max(sizeof(X3DBuffer), sizeof(ConstBuffer)), sizeof(ByteAddressBuffer));
	static const size_t POOL_ALLOCATION_ALIGN = core::Max(core::Max(X_ALIGN_OF(X3DBuffer), X_ALIGN_OF(ConstBuffer)), X_ALIGN_OF(ByteAddressBuffer));


	typedef render::Handle BufferHandle;
	typedef VertexBufferHandle VertexBufferHandle;
	typedef IndexBufferHandle IndexBufferHandle;
	typedef ConstantBufferHandle ConstantBufferHandle;

	typedef core::MemoryArena<core::PoolAllocator,
		core::SingleThreadPolicy,
#if X_DEBUG
		core::SimpleBoundsChecking,
		core::NoMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif !X_DEBUG
	> PoolArena;


	struct Stats
	{
		Stats();

		uint32_t numIndexBuffers;
		uint32_t numVertexBuffers;
		uint32_t numConstBuffers;
		uint32_t maxIndexBuffers;
		uint32_t maxVertexBuffers;
		uint32_t maxConstBuffers;

		uint32_t indexesBytes;
		uint32_t vertexBytes;
		uint32_t constBufferBytes;
		uint32_t maxIndexesBytes;
		uint32_t maxVertexBytes;
		uint32_t maxConstBufferBytes;
	};

public:
	BufferManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice, 
		ContextManager& contextMan, DescriptorAllocator& descriptorAllocator);
	~BufferManager();

	bool init(void);
	void shutDown(void);

	VertexBufferHandle createVertexBuf(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag);
	IndexBufferHandle createIndexBuf(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag);
	ConstantBufferHandle createConstBuf(uint32_t size, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag);

	// free from ID
	void freeIB(IndexBufferHandle IBHandle);
	void freeVB(VertexBufferHandle VBHandle);
	void freeCB(VertexBufferHandle CBHandle);

	// get the buffer from a ID
	static X_INLINE X3DBuffer* IBFromHandle(IndexBufferHandle bufHandle);
	static X_INLINE X3DBuffer* VBFromHandle(VertexBufferHandle bufHandle);
	static X_INLINE ConstBuffer* CBFromHandle(ConstantBufferHandle bufHandle);

	
	X_INLINE bool ValidHandle(VertexBufferHandle handle) const;


	void getBufSize(BufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize) const;

	Stats getStats(void) const;

private:
	// Internal create
	X3DBuffer* Int_CreateVB(uint32_t size);
	X3DBuffer* Int_CreateIB(uint32_t size);
	ConstBuffer* Int_CreateCB(uint32_t size);

	X_INLINE BufferHandle createHandleForBuffer(X3DBuffer* pBuf);
	static X_INLINE X3DBuffer* bufferForHandle(BufferHandle handle);
	static X_INLINE ConstBuffer* constBufferForHandle(BufferHandle handle);

private:
	ID3D12Device* pDevice_;
	ContextManager& contextMan_;
	DescriptorAllocator& descriptorAllocator_;

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