#pragma once

#ifndef X_EXTENDEDMEMORYTRACKING_H_
#define X_EXTENDEDMEMORYTRACKING_H_

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#include "Memory/HeapArea.h"
#include "Memory/MemoryArena.h"
#include "Memory/AllocationPolicies/LinearAllocator.h"
#include "Memory/ThreadPolicies/SingleThreadPolicy.h"
#include "Memory/BoundsCheckingPolicies/NoBoundsChecking.h"
#include "Memory/MemoryTaggingPolicies/NoMemoryTagging.h"
#include "Memory/MemoryTrackingPolicies/NoMemoryTracking.h"
// #include "Memory/MemoryTrackingPolicies/SimpleMemoryTracking.h"
#include "Containers\HashMap.h"
// #include "Containers\HashMapHashFunctions.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a tracking policy for memory arenas.
/// \details This class implements the concepts of a tracking policy as expected by the MemoryArena class. It stores
/// each allocation along with additional information in a hash map. Whenever a leak is reported, the tracker provides
/// information about the location of the leak (source file and line number), as well as the allocation's ID, its size,
/// and other bits of info.
///
/// This kind of tracker causes quite a bit of allocation overhead for each tracked allocation. Thus, it should probably
/// only be used to track down the location of a leak once it has been reported by a simpler memory tracker. However, if
/// the application has enough memory to spare, always using this tracker is a perfectly valid option as well.
/// \sa NoMemoryTracking SimpleMemoryTracking FullMemoryTracking
class ExtendedMemoryTracking
{
	/// \brief The data stored for each allocation.
	struct AllocationData
	{
		/// Default constructor.
		AllocationData(size_t originalSize, size_t internalSize
			X_MEM_HUMAN_IDS_CB(const char* ID)
			X_MEM_HUMAN_IDS_CB(const char* typeName)
			X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), const char* memoryArenaName);

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

	private:
		X_NO_ASSIGN(AllocationData);
	};

	/// A type representing the hash map that is used to store allocation data.
	// typedef HashMap<void*, AllocationData, &hashMapHashFunction::PointerType<void> > AllocationTable;

	typedef HashMap<void*, AllocationData > AllocationTable;


	/// The type of arena that is used to allocate memory for the hash map.
	typedef MemoryArena<LinearAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> LinearArena;

public:
	/// A human-readable string literal containing the policy's type.
	static const char* const TYPE_NAME;

	/// Defines the amount of overhead that each allocation causes.
	static const size_t PER_ALLOCATION_OVERHEAD = AllocationTable::PER_ENTRY_SIZE;

	/// Default constructor.
	ExtendedMemoryTracking(void);

	/// Default destructor, reporting all detected leaks.
	~ExtendedMemoryTracking(void);

	/// Stores the allocation along with additional data in a hash map.
	void OnAllocation(void* memory, size_t originalSize, size_t internalSize, size_t alignment, size_t offset
		X_MEM_HUMAN_IDS_CB(const char* ID)
		X_MEM_HUMAN_IDS_CB(const char* typeName)
		X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo),
		const char* memoryArenaName);

	/// Removes the allocation data from a hash map.
	void OnDeallocation(void* memory);

private:
	X_NO_COPY(ExtendedMemoryTracking);
	X_NO_ASSIGN(ExtendedMemoryTracking);

	unsigned int numAllocations_;

	HeapArea heapArea_;
	LinearAllocator allocator_;
	LinearArena arena_;
	AllocationTable table_;
};


X_NAMESPACE_END


#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#endif // !X_EXTENDEDMEMORYTRACKING_H_
