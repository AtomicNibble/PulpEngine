#pragma once

X_NAMESPACE_BEGIN(render)

// -------------------------------------------

X_INLINE CommandQue& CommandListManger::getGraphicsQueue(void)
{
    return graphicsQueue_;
}

X_INLINE CommandQue& CommandListManger::getComputeQueue(void)
{
    return computeQueue_;
}

X_INLINE CommandQue& CommandListManger::getCopyQueue(void)
{
    return copyQueue_;
}

X_INLINE CommandQue& CommandListManger::getQueue(D3D12_COMMAND_LIST_TYPE Type)
{
    switch (Type) {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return computeQueue_;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return copyQueue_;
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return graphicsQueue_;
#if X_DEBUG
        default:
            X_ASSERT_UNREACHABLE();
            return graphicsQueue_;
#else
            X_NO_SWITCH_DEFAULT;
#endif
    }

    return graphicsQueue_;
}

X_INLINE void CommandListManger::createNewCommandList(D3D12_COMMAND_LIST_TYPE type,
    ID3D12GraphicsCommandList** pListOut, ID3D12CommandAllocator** pAllocatorOut)
{
    createNewCommandList(type, nullptr, pListOut, pAllocatorOut);
}

X_INLINE ID3D12CommandQueue* CommandListManger::getCommandQueue(void)
{
    return graphicsQueue_.getCommandQueue();
}

X_INLINE bool CommandListManger::isFenceComplete(uint64_t fenceValue)
{
    CommandQue& que = getQueue(D3D12_COMMAND_LIST_TYPE(fenceValue >> 56));

    return que.isFenceComplete(fenceValue);
}

X_INLINE void CommandListManger::waitForFence(uint64_t fenceValue)
{
    CommandQue& que = getQueue(D3D12_COMMAND_LIST_TYPE(fenceValue >> 56));

    que.waitForFence(fenceValue);
}

X_INLINE void CommandListManger::idleGPU(void)
{
    graphicsQueue_.waitForIdle();
    computeQueue_.waitForIdle();
    copyQueue_.waitForIdle();
}

X_NAMESPACE_END
