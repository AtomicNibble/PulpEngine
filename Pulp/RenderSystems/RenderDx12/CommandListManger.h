#pragma once

#ifndef X_COMMAND_QUE_H_
#define X_COMMAND_QUE_H_

#include "CommandQue.h"

X_NAMESPACE_BEGIN(render)

class PSO;

class CommandListManger
{
public:
    CommandListManger(core::MemoryArenaBase* arena);
    ~CommandListManger();

    bool create(ID3D12Device* pDevice);
    void shutdown(void);

    X_INLINE CommandQue& getGraphicsQueue(void);
    X_INLINE CommandQue& getComputeQueue(void);
    X_INLINE CommandQue& getCopyQueue(void);
    X_INLINE CommandQue& getQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    X_INLINE ID3D12CommandQueue* getCommandQueue(void);

    void createNewCommandList(D3D12_COMMAND_LIST_TYPE type, PSO& initialPso,
        ID3D12GraphicsCommandList** pListOut, ID3D12CommandAllocator** pAllocatorOut);
    void createNewCommandList(D3D12_COMMAND_LIST_TYPE type,
        ID3D12GraphicsCommandList** pListOut, ID3D12CommandAllocator** pAllocatorOut);

    X_INLINE bool isFenceComplete(uint64_t fenceValue);
    X_INLINE void waitForFence(uint64_t fenceValue);
    X_INLINE void idleGPU(void);

private:
    void createNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12PipelineState* pInitialPso,
        ID3D12GraphicsCommandList** pListOut, ID3D12CommandAllocator** pAllocatorOut);

private:
    ID3D12Device* pDevice_;

    CommandQue graphicsQueue_;
    CommandQue computeQueue_;
    CommandQue copyQueue_;
};

X_NAMESPACE_END

#include "CommandListManger.inl"

#endif // !X_COMMAND_QUE_H_