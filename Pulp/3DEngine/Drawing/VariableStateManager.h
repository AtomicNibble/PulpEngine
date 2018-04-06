#pragma once

#include <IShader.h>

X_NAMESPACE_DECLARE(render,
    namespace Commands {
        struct ResourceStateBase;
    } // namespace
);

X_NAMESPACE_BEGIN(engine)

class VariableStateManager
{
    typedef core::MemoryArena<
        core::PoolAllocator,
        core::MultiThreadPolicy<core::Spinlock>, // allow multi thread creation.

#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::NoMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES
        >
        PoolArena;

    struct Stats
    {
        Stats();

        uint32_t numVariablestates;
        uint32_t maxVariablestates;

        uint32_t numTexStates;
        uint32_t numSamplers;
        uint32_t numCBS;
        uint32_t numBuffers;
    };

public:
    static const uint32_t MAX_CONST_BUFFERS = render::MAX_CONST_BUFFERS_BOUND;
    static const uint32_t MAX_TEX_STATES = render::MAX_TEXTURES_BOUND;

    static const uint32_t MAX_STATES = 1024;

public:
    VariableStateManager();
    ~VariableStateManager();

    void shutDown(void);

    render::Commands::ResourceStateBase* createVariableState(size_t numTexStates, size_t numSamp, size_t numCBs, size_t numBuf);
    void releaseVariableState(render::Commands::ResourceStateBase* pVS);

    Stats getStats(void) const;

private:
    render::Commands::ResourceStateBase* createVariableState_Interal(int8_t numTexStates, int8_t numSamp, int8_t numCBs, int8_t numBuf);

private:
    static constexpr size_t allocSize(int8_t numTexStates, int8_t numSamp, int8_t numCBs, int8_t numBuf);

private:
    // do i want linera allocator which i think would work well or a pool.
    // down side of lineraalocator is i can't free them so for now we pool.
    // could create multiple pools to help reduce wastage.
    core::HeapArea statePoolHeap_;
    core::PoolAllocator statePoolAllocator_;
    PoolArena statePool_;

    render::Commands::ResourceStateBase* pEmtpyState_;

private:
#if X_ENABLE_VARIABLE_STATE_STATS
    mutable core::Spinlock statsLock_;
    Stats stats_;
#endif // !X_ENABLE_VARIABLE_STATE_STATS
};

X_NAMESPACE_END