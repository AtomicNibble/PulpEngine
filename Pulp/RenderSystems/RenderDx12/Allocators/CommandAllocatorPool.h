#pragma once

#ifndef X_COMMAND_ALLOCATOR_POOL_H_
#define X_COMMAND_ALLOCATOR_POOL_H_

#include <Containers\Array.h>
#include <Containers\Fifo.h>
#include <Threading\CriticalSection.h>

X_NAMESPACE_BEGIN(render)

class CommandAllocatorPool
{
public:
    CommandAllocatorPool(core::MemoryArenaBase* arena, // use for pointer sotrage, nothing todo with actual command allocations.
        D3D12_COMMAND_LIST_TYPE Type);
    ~CommandAllocatorPool();

    void create(ID3D12Device* pDevice);
    void shutdown(void);

    ID3D12CommandAllocator* requestAllocator(uint64_t CompletedFenceValue);
    void discardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* pAllocator);

    X_INLINE size_t size(void) const;

private:
    ID3D12Device* pDevice_;
    const D3D12_COMMAND_LIST_TYPE commandListType_;

    core::Array<ID3D12CommandAllocator*> allocatorPool_;
    core::Fifo<std::pair<uint64_t, ID3D12CommandAllocator*>> readyAllocator_;
    core::CriticalSection cs_;
};

X_NAMESPACE_END

#include "CommandAllocatorPool.inl"

#endif // !X_COMMAND_ALLOCATOR_POOL_H_