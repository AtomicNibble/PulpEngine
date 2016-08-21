#include "stdafx.h"
#include "BufferManager.h"


X_NAMESPACE_BEGIN(render)

BufferManager::Stats::Stats()
{
	core::zero_this(this);
}

BufferManager::BufferManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
	pDevice_(pDevice)
{
	X_UNUSED(arena);
}

BufferManager::~BufferManager()
{

}

void* BufferManager::createVertexBuf(uint32_t size, const void* pInitialData, IRender::CpuAccessFlags accessFlag)
{

	return nullptr;
}


void* BufferManager::createIndexBuf(uint32_t size, const void* pInitialData, IRender::CpuAccessFlags accessFlag)
{

	return nullptr;
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




X_NAMESPACE_END

