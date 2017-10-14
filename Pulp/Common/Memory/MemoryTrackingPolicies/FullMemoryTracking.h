#pragma once

#ifndef X_FULLMEMORYTRACKING_H_
#define X_FULLMEMORYTRACKING_H_

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#include "Memory/HeapArea.h"
#include "Memory/MemoryArena.h"
#include "Memory/AllocationPolicies/LinearAllocator.h"
#include "Memory/ThreadPolicies/SingleThreadPolicy.h"
#include "Memory/BoundsCheckingPolicies/NoBoundsChecking.h"
#include "Memory/MemoryTaggingPolicies/NoMemoryTagging.h"
#include "Memory/MemoryTrackingPolicies/NoMemoryTracking.h"
#include "Containers/HashMap.h"
//#include "Containers/HashMapHashFunctions.h"
#include "Debugging/CallStack.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a tracking policy for memory arenas.
/// \details This class implements the concepts of a tracking policy as expected by the MemoryArena class. It is similar
/// to the ExtendedMemoryTracking policy, but additionally stores a complete call stack for each allocation. Because of
/// the additional overhead caused by storing the call stack, this kind of tracker should be used only in circumstances
/// where the root cause of a memory leak is hard to find.
/// \sa NoMemoryTracking SimpleMemoryTracking ExtendedMemoryTracking
class FullMemoryTracking
{
	/// \brief The data stored for each allocation.
	struct AllocationData
	{
		/// Default constructor.
		AllocationData(size_t originalSize, size_t internalSize
			X_MEM_HUMAN_IDS_CB(const char* ID)
			X_MEM_HUMAN_IDS_CB(const char* typeName)
			X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo),
			const char* memoryArenaName, const CallStack& callStack);

		size_t originalSize_;
		size_t internalSize_;
#if X_ENABLE_MEMORY_HUMAN_IDS
		const char* ID_;
		const char* typeName_;
#endif // !X_ENABLE_MEMORY_HUMAN_IDS
#if X_ENABLE_MEMORY_SOURCE_INFO
		SourceInfo sourceInfo_;
#endif // !X_ENABLE_MEMORY_SOURCE_INFO
		const char* memoryArenaName_;
		CallStack callStack_;

	private:
		X_NO_ASSIGN(AllocationData);
	};

	/// A type representing the hash map that is used to store allocation data.
//	typedef HashMap<void*, AllocationData, &hashMapHashFunction::PointerType<void> > AllocationTable;
	typedef HashMap<void*, AllocationData> AllocationTable;

	/// The type of arena that is used to allocate memory for the hash map.
	typedef MemoryArena<LinearAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> LinearArena;

public:
	/// A human-readable string literal containing the policy's type.
	static const char* const TYPE_NAME;

	/// Defines the amount of overhead that each allocation causes.
	static const size_t PER_ALLOCATION_OVERHEAD = AllocationTable::PER_ENTRY_SIZE;

	/// Default constructor.
	FullMemoryTracking(void);

	/// Default destructor, reporting all detected leaks.
	~FullMemoryTracking(void);

	/// Stores the allocation along with additional data and a call stack in a hash map.
	void OnAllocation(void* memory, size_t originalSize, size_t internalSize, size_t alignment, size_t offset
		X_MEM_HUMAN_IDS_CB(const char* ID)
		X_MEM_HUMAN_IDS_CB(const char* typeName)
		X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo),
		const char* memoryArenaName);

	/// Removes the allocation data from a hash map.
	void OnDeallocation(void* memory);

private:
	X_NO_COPY(FullMemoryTracking);
	X_NO_ASSIGN(FullMemoryTracking);

	unsigned int numAllocations_;

	HeapArea heapArea_;
	LinearAllocator allocator_;
	LinearArena arena_;
	AllocationTable table_;
};

X_NAMESPACE_END


#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#endif // !X_FULLMEMORYTRACKING_H_
