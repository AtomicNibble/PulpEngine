#pragma once

X_NAMESPACE_BEGIN(level)

template<typename ArenaType>
struct GrowingPool
{
    GrowingPool(size_t maxAllocsize, size_t maxAlign, size_t maxAllocs, size_t growSize) :
        allocator_(
            core::bitUtil::NextPowerOfTwo(
                ArenaType::getMemoryRequirement(maxAllocsize) * maxAllocs),
            core::bitUtil::NextPowerOfTwo(
                ArenaType::getMemoryRequirement(growSize)),
            0,
            ArenaType::getMemoryRequirement(maxAllocsize),
            ArenaType::getMemoryAlignmentRequirement(maxAlign),
            ArenaType::getMemoryOffsetRequirement()),
        arena_(&allocator_, "PoolArena")
    {
    }

    core::GrowingPoolAllocator allocator_;
    ArenaType arena_;
};

X_NAMESPACE_END
