#include "EngineCommon.h"
#include "ExtendedMemoryTracking.h"
#include "memory\VirtualMem.h"


#if X_ENABLE_MEMORY_DEBUG_POLICIES

X_NAMESPACE_BEGIN(core)


	/// \brief The data stored for each allocation.
ExtendedMemoryTracking::AllocationData::AllocationData(size_t originalSize, size_t internalSize, const char* ID, const char* typeName, const SourceInfo& sourceInfo, const char* memoryArenaName) :
	originalSize_(originalSize),
	internalSize_(internalSize),
	ID_(ID),
	typeName_(typeName),
	sourceInfo_(sourceInfo),
	memoryArenaName_(memoryArenaName)
{
}



const char* const ExtendedMemoryTracking::TYPE_NAME = "ExtendedTracking";

ExtendedMemoryTracking::ExtendedMemoryTracking(void) :
	numAllocations_( 0 ),
	heapArea_(
		bitUtil::RoundUpToMultiple<size_t>(
			AllocationTable::GetMemoryRequirement<LinearArena>(10000), VirtualMem::GetPageSize()
		)
	),
	
	allocator_(heapArea_.start(), heapArea_.end()),
	arena_(&allocator_, "ExtendedMemoryTracking"),
	table_(&arena_, 9999)
{

}

ExtendedMemoryTracking::~ExtendedMemoryTracking(void)
{
	if (this->numAllocations_)
	{
	//	X_ASSERT(numAllocations_ == 0, "Memory leaks detected.")(numAllocations_);

		AllocationTable::const_iterator it = table_.begin();
		AllocationTable::const_iterator end = table_.end();
		size_t Num = 1;

		X_DISABLE_WARNING(4127)
		while (true)
		X_ENABLE_WARNING(4127)
		{
			if( it == end )
				break;

			const AllocationData& info = it->second; // m_value;

			X_ERROR( "ExMemTracking", "Unfreed allocation %d at address 0x%08p:", Num, it->first );

			{
				X_LOG_BULLET;

				X_LOG0( "ExMemTracking", "Arena name: \"%s\"", info.memoryArenaName_ );
				X_LOG0( "ExMemTracking", "Allocation ID: \"%s\"", info.ID_ );
				X_LOG0( "ExMemTracking", "Type name: \"%s\"", info.typeName_ );
				X_LOG0( "ExMemTracking", "Original size: %d", info.originalSize_ );
			//	X_LOG0( "ExMemTracking", "Allocated size: %d", info.);

				const SourceInfo& sourceInfo = info.sourceInfo_;

				X_LOG0( "ExMemTracking", "Filename(line): \"%s(%d)\"", sourceInfo.file_, sourceInfo.line_ );
				X_LOG0( "ExMemTracking", "Function: \"%s\"", sourceInfo.function_ );
				X_LOG0( "ExMemTracking", "Function signature: \"%s\"", sourceInfo.functionSignature_ );
			}
			++it;
			Num++;
		}
	}

	// o dear o.o
	X_ASSERT( numAllocations_ == 0, "Memory leaks detected. Num: %d", numAllocations_ )();
}



/// Stores the allocation along with additional data in a hash map.
void ExtendedMemoryTracking::OnAllocation(void* memory, size_t originalSize, size_t internalSize, 
	size_t alignment, size_t offset, const char* ID, const char* typeName, 
	const SourceInfo& sourceInfo, const char* memoryArenaName)
{
	X_UNUSED(alignment);
	X_UNUSED(offset);

	numAllocations_++;
	table_.insert(std::make_pair(memory, AllocationData(originalSize, internalSize, ID, typeName, sourceInfo, memoryArenaName)));
}


/// Removes the allocation data from a hash map.
void ExtendedMemoryTracking::OnDeallocation(void* memory)
{
	numAllocations_--;
	table_.erase(memory);
}



X_NAMESPACE_END


#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES
