#pragma once



X_NAMESPACE_BEGIN(render)

// all like vertex and index buffers we give back to 3dengine should come from here
// then the 3dengine can just pass back the handle and we understand it.
// i might change if from a handle to a interface.
// so the the 3d engine has a way to update stuff.
// or maybe it's better if the 2d engine has to do it via the render system.
// then i might be able to track changes a bit better to resources.

class BufferManager
{
public:
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


	X_INLINE void* createVertexBuf(uint32_t size, IRender::CpuAccessFlags accessFlag);
	void* createVertexBuf(uint32_t size, const void* pInitialData, IRender::CpuAccessFlags accessFlag);
	X_INLINE void* createIndexBuf(uint32_t size, IRender::CpuAccessFlags accessFlag);
	void* createIndexBuf(uint32_t size, const void* pInitialData, IRender::CpuAccessFlags accessFlag);


	Stats getStats(void) const;


private:
	ID3D12Device* pDevice_;

private:
#if VID_MEMORY_STATS
	Stats stats_;
#endif // !VID_MEMORY_STATS
};

X_INLINE void* BufferManager::createVertexBuf(uint32_t size, IRender::CpuAccessFlags accessFlag)
{
	return createVertexBuf(size, nullptr, accessFlag);
}

X_INLINE void* BufferManager::createIndexBuf(uint32_t size, IRender::CpuAccessFlags accessFlag)
{
	return createIndexBuf(size, nullptr, accessFlag);
}

X_NAMESPACE_END
