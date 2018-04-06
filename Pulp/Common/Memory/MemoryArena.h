#pragma once

#ifndef X_MEMORYARENA_H_
#define X_MEMORYARENA_H_

#include "Memory/MemoryArenaBase.h"
#include "Util/BitUtil.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief Default policy-based implementation of a memory arena.
/// \details This class implements the MemoryArenaBase interface, and defers almost all of the functionality to different
/// policy-classes. Using a policy-based approach, completely different types of arenas can be defined by using a single-line
/// typedef.
///
/// The memory arena implementation consists of five policies:
/// - An allocation policy.
/// - A bounds checking policy.
/// - A memory tagging policy.
/// - A memory tracking policy.
/// - A threading policy.
///
/// The allocation policy is responsible for allocating raw memory. There are several different allocation policies
/// available, both non-growing and growing variants.
///
/// The bounds checking policy checks for memory overwrites at the front or back of an allocation. Two different policies
/// are available by default: \ref NoBoundsChecking and \ref SimpleBoundsChecking.
///
/// The memory tagging policy is responsible for marking each allocated and freed region of memory with a specific bit
/// pattern. Available policies are the \ref NoMemoryTagging and \ref SimpleMemoryTagging.
///
/// The memory tracking policy tracks allocations and reports unfreed ones. Several different policies are available
/// by default: \ref NoMemoryTracking, \ref SimpleMemoryTracking, \ref ExtendedMemoryTracking and \ref FullMemoryTracking.
///
/// The threading policy dictates whether a memory arena is single-threaded or multi-threaded. Available policies are the
/// \ref SingleThreadPolicy and \ref MultiThreadPolicy.
///
/// One of the advantages of policy-based design is that users can implement custom policies that follow
/// the concepts expected by this class. Different policies - both those available by default and custom ones - can be
/// freely mixed and matched simply by defining new classes of arenas using a typedef:
/// \code
///   // defines a single-threaded arena that uses a pool allocator, and simple bounds checking, tracking, and tagging.
///   typedef core::MemoryArena<core::PoolAllocator, core::SingleThreadPolicy, core::SimpleBoundsChecking, core::SimpleMemoryTracking, core::SimpleMemoryTagging> PoolArena;
///
///   // defines a single-threaded arena that uses a custom allocator, full-fledged memory tracking, but no tagging or bounds checking.
///   typedef core::MemoryArena<CustomAllocator, core::SingleThreadPolicy, core::NoBoundsChecking, core::FullMemoryTracking, core::NoMemoryTagging> CustomArena;
/// \endcode
/// This severely reduces the amount of code that has to be written in order to customize a certain aspect of an arena.
///
/// For more information about the advantages and disadvantages of policy-based design, see http://www.altdevblogaday.com/2011/11/28/policy-based-design-in-c/.
/// \sa MemoryArenaBase SimpleMemoryArena MemoryArenaStatistics X_NEW X_NEW_ARRAY X_DELETE X_DELETE_ARRAY
template<class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
class MemoryArena : public MemoryArenaBase
{
public:
    /// A simple typedef that introduces the template type into this class.
    typedef AllocationPolicy AllocationPolicy;

    /// A simple typedef that introduces the template type into this class.
    typedef ThreadPolicy ThreadPolicy;

    /// A simple typedef that introduces the template type into this class.
    typedef BoundsCheckingPolicy BoundsCheckingPolicy;

    /// A simple typedef that introduces the template type into this class.
    typedef MemoryTrackingPolicy MemoryTrackingPolicy;

    /// A simple typedef that introduces the template type into this class.
    typedef MemoryTaggingPolicy MemoryTaggingPolicy;

    static const bool IS_THREAD_SAFE = ThreadPolicy::IS_THREAD_SAFE;

    MemoryArena(AllocationPolicy* allocator, const char* name);

    /// Empty destructor.
    virtual ~MemoryArena(void);

    /// \brief Allocates raw memory that satisfies the alignment requirements.
    /// \details All arguments provided for debugging purposes are passed along to the respective policies.
    virtual void* allocate(size_t size, size_t alignment, size_t offset X_MEM_HUMAN_IDS_CB(const char* ID) X_MEM_HUMAN_IDS_CB(const char* typeName) X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo)) X_FINAL;

    void free(void* ptr) X_FINAL;
    void free(void* ptr, size_t size) X_FINAL;

    size_t getSize(void* ptr) X_FINAL;

    size_t usableSize(void* ptr) const X_FINAL;

    /// Returns statistics regarding the allocations made by the memory arena.
    MemoryArenaStatistics getStatistics(void) const X_FINAL;

    MemoryAllocatorStatistics getAllocatorStatistics(bool children = false) const X_FINAL;

    bool isThreadSafe(void) const X_FINAL;

    /// Returns the memory requirement for an allocation of \a size, taking into account bounds checking and other potential overhead.
    static constexpr inline size_t getMemoryRequirement(size_t size);

    /// Returns the alignment requirement for an allocation requiring \a alignment, taking into account bounds checking and other potential overhead.
    static constexpr inline size_t getMemoryAlignmentRequirement(size_t alignment);

    /// Returns the offset requirement for an allocation, taking into account bounds checking and other potential overhead.
    static constexpr inline size_t getMemoryOffsetRequirement(void);

private:
    X_NO_COPY(MemoryArena);
    X_NO_ASSIGN(MemoryArena);

    AllocationPolicy* allocator_;
    ThreadPolicy threadGuard_;
    BoundsCheckingPolicy boundsChecker_;
    MemoryTrackingPolicy memoryTracker_;
    MemoryTaggingPolicy memoryTagger_;

    const char* name_;

#if X_ENABLE_MEMORY_ARENA_STATISTICS
    MemoryArenaStatistics statistics_;
#endif
};

#include "MemoryArena.inl"

X_NAMESPACE_END

#endif
