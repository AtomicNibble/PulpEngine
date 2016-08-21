#include "stdafx.h"
#include "BufferManager.h"


X_NAMESPACE_BEGIN(render)

BufferManager::Stats::Stats()
{
	core::zero_this(this);
}

BufferManager::BufferManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
	pDevice_(pDevice),
	idLookup_(arena),
	freeIds_(arena)
{
	X_UNUSED(arena);
}

BufferManager::~BufferManager()
{

}

BufferManager::VertexBufferHandle BufferManager::createVertexBuf(uint32_t size, const void* pInitialData,
	IRender::BufUsage::Enum usage, IRender::CpuAccessFlags accessFlag)
{

	return 0;
}


BufferManager::IndexBufferHandle BufferManager::createIndexBuf(uint32_t size, const void* pInitialData,
	IRender::BufUsage::Enum usage, IRender::CpuAccessFlags accessFlag)
{

	return 0;
}

void BufferManager::freeIB(IndexBufferHandle IBHandle)
{
	X_UNUSED(IBHandle);
}

void BufferManager::freeVB(VertexBufferHandle VBHandle)
{
	X_UNUSED(VBHandle);
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

