#pragma once

#include <Threading\CriticalSection.h>
#include "Allocators\CommandAllocatorPool.h"

X_NAMESPACE_BEGIN(render)

class CommandQue
{
public:
    CommandQue(core::MemoryArenaBase* arena, D3D12_COMMAND_LIST_TYPE type);
    ~CommandQue();

    bool create(ID3D12Device* pDevice);
    void shutdown(void);

    uint64_t incrementFence(void);
    X_INLINE uint64_t getNextFenceValue(void) const;
    bool isFenceComplete(uint64_t fenceValue);
    void stallForProducer(CommandQue& producer);
    void waitForFence(uint64_t fenceValue);
    X_INLINE void waitForIdle(void);

    uint64_t executeCommandList(ID3D12CommandList* pList);

    X_INLINE ID3D12CommandQueue* getCommandQueue(void);
    ID3D12CommandAllocator* requestAllocator(void);
    void discardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

private:
    ID3D12CommandQueue* pCommandQueue_;
    ID3D12Fence* pFence_;
    const D3D12_COMMAND_LIST_TYPE type_;
    CommandAllocatorPool allocatorPool_;

    core::CriticalSection eventCs_;
    core::CriticalSection fenceCs_;

    uint64_t nextFenceValue_;
    uint64_t lastCompletedFenceValue_;
    HANDLE fenceEventHandle_;
};

X_NAMESPACE_END

#include "CommandQue.inl"