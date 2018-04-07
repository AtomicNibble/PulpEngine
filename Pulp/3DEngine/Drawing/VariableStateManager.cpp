#include "stdafx.h"
#include "VariableStateManager.h"

#include <IRenderCommands.h>

X_NAMESPACE_BEGIN(engine)

namespace
{
    // didnt make these static members as Vs won't show me the size when i hover over :(

    const uint32_t MAX_ALIGN = X_ALIGN_OF(render::Commands::ResourceStateBase);
    const uint32_t MAX_ALOC_SIZE = render::Commands::ResourceStateBase::getMaxStateSize();

    // used to access the protected members.
    // thought id try somthing diffrent than making the manager a friend :|
    struct ResourceStateInit : public render::Commands::ResourceStateBase
    {
#if !X_COMPILER_CLANG
        static const size_t lastMemberEnd = X_OFFSETOF(ResourceStateBase, numBuffers) + sizeof(ResourceStateBase::numBuffers);
        // make sure that the compiler did not add in padding after last member.
        static_assert(sizeof(ResourceStateBase) == lastMemberEnd, "Compiler added paddin at end");
#endif // !X_COMPILER_CLANG

        void setSizes(int8_t numTex, int8_t numSamp, int8_t numCBs, int8_t numBuf)
        {
            numTextStates = numTex;
            numSamplers = numSamp;
            numCbs = numCBs;
            numBuffers = numBuf;
        }
    };

} // namespace

VariableStateManager::Stats::Stats()
{
    core::zero_this(this);
}

// ---------------------------------------

VariableStateManager::VariableStateManager() :
    statePoolHeap_(
        core::bitUtil::RoundUpToMultiple<size_t>(
            PoolArena::getMemoryRequirement(MAX_ALOC_SIZE) * MAX_STATES,
            core::VirtualMem::GetPageSize())),
    statePoolAllocator_(statePoolHeap_.start(), statePoolHeap_.end(),
        PoolArena::getMemoryRequirement(MAX_ALOC_SIZE),
        PoolArena::getMemoryAlignmentRequirement(MAX_ALIGN),
        PoolArena::getMemoryOffsetRequirement()),
    statePool_(&statePoolAllocator_, "StatePool"),
    pEmtpyState_(nullptr)
{
    g_3dEngineArena->addChildArena(&statePool_);

    pEmtpyState_ = createVariableState_Interal(0, 0, 0, 0);
}

VariableStateManager::~VariableStateManager()
{
}

void VariableStateManager::shutDown(void)
{
    if (pEmtpyState_) {
        releaseVariableState(pEmtpyState_);
    }
}

render::Commands::ResourceStateBase* VariableStateManager::createVariableState(size_t numTexStates, size_t numSamp, size_t numCBs, size_t numBuf)
{
    static_assert(core::compileTime::IsPOD<render::Commands::ResourceStateBase>::Value, "ResourceStateBase must be pod");

    // return the same state for empty ones.
    // reduces allocations.
    // and increases chance of cache hit for empty ones as same memory.
    if (numTexStates == 0 && numSamp == 0 && numCBs == 0 && numBuf == 0) {
        return pEmtpyState_;
    }

    return createVariableState_Interal(
        safe_static_cast<int8_t>(numTexStates), safe_static_cast<int8_t>(numSamp),
        safe_static_cast<int8_t>(numCBs), safe_static_cast<int8_t>(numBuf));
}

void VariableStateManager::releaseVariableState(render::Commands::ResourceStateBase* pVS)
{
    X_ASSERT_NOT_NULL(pVS);

#if X_ENABLE_VARIABLE_STATE_STATS
    {
        core::ScopedLock<decltype(statsLock_)> lock(statsLock_);

        --stats_.numVariablestates;
        stats_.numTexStates += pVS->getNumTextStates();
        stats_.numSamplers += pVS->getNumSamplers();
        stats_.numCBS += pVS->getNumCBs();
    }
#endif // !X_ENABLE_VARIABLE_STATE_STATS

    X_DELETE(pVS, &statePool_);
}

render::Commands::ResourceStateBase* VariableStateManager::createVariableState_Interal(int8_t numTexStates, int8_t numSamp,
    int8_t numCBs, int8_t numBuf)
{
    static_assert(core::compileTime::IsPOD<render::Commands::ResourceStateBase>::Value, "ResourceStateBase must be pod");

#if X_ENABLE_VARIABLE_STATE_STATS
    {
        core::ScopedLock<decltype(statsLock_)> lock(statsLock_);

        ++stats_.numVariablestates;
        stats_.maxVariablestates = core::Max(stats_.maxVariablestates, stats_.numVariablestates);
        stats_.numTexStates += numTexStates;
        stats_.numSamplers += numSamp;
        stats_.numCBS += numCBs;
        stats_.numBuffers += numBuf;
    }
#endif // !X_ENABLE_VARIABLE_STATE_STATS

    const size_t requiredBytes = allocSize(numTexStates, numSamp, numCBs, numBuf);
    void* pData = statePool_.allocate(requiredBytes, MAX_ALIGN, 0 X_MEM_IDS("State", "ResourceStateBase") X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO));

    auto* pState = reinterpret_cast<ResourceStateInit*>(pData);

    pState->setSizes(numTexStates, numSamp, numCBs, numBuf);

    return pState;
}

X_INLINE constexpr size_t VariableStateManager::allocSize(int8_t numTexStates, int8_t numSamp, int8_t numCBs, int8_t numBuf)
{
    return sizeof(render::Commands::ResourceStateBase) + (sizeof(render::TextureState) * numTexStates) + (sizeof(render::SamplerState) * numSamp) + (sizeof(render::ConstantBufferHandle) * numCBs) + (sizeof(render::BufferState) * numBuf);
}

VariableStateManager::Stats VariableStateManager::getStats(void) const
{
#if X_ENABLE_VARIABLE_STATE_STATS
    core::ScopedLock<decltype(statsLock_)> lock(statsLock_);

    return stats_;
#else
    static Stats stats;
    return stats;
#endif // !X_ENABLE_VARIABLE_STATE_STATS
}

X_NAMESPACE_END