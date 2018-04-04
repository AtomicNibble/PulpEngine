#include "EngineCommon.h"
#include "FullMemoryTracking.h"
#include "memory\VirtualMem.h"

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

X_NAMESPACE_BEGIN(core)


FullMemoryTracking::AllocationData::AllocationData(
	size_t originalSize, size_t internalSize
	X_MEM_HUMAN_IDS_CB(const char* ID)
	X_MEM_HUMAN_IDS_CB(const char* typeName)
	X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo),
	const char* memoryArenaName, const CallStack& callStack) :
	originalSize_(originalSize),
	internalSize_(internalSize),
#if X_ENABLE_MEMORY_SOURCE_INFO
	sourceInfo_(sourceInfo),
#endif // !X_ENABLE_MEMORY_SOURCE_INFO
	memoryArenaName_(memoryArenaName),
	callStack_(callStack)
{
#if X_ENABLE_MEMORY_HUMAN_IDS
	ID_ = ID;
	typeName_ = typeName;
#endif // !X_ENABLE_MEMORY_HUMAN_IDS
}


const char* const FullMemoryTracking::TYPE_NAME = "FullMemoryTracking";


FullMemoryTracking::FullMemoryTracking(void) :
	numAllocations_(0),

	heapArea_(
		bitUtil::RoundUpToMultiple<size_t>(
		AllocationTable::GetMemoryRequirement<LinearArena>(40000), VirtualMem::GetPageSize()
		)
	),

	allocator_(heapArea_.start(), heapArea_.end()),
	arena_(&allocator_, "FullMemoryTracking"),
	table_(&arena_,20000)
{

}


FullMemoryTracking::~FullMemoryTracking(void)
{
	if (this->numAllocations_ != 0)
	{
		AllocationTable::const_iterator it = table_.begin();
		AllocationTable::const_iterator end = table_.end();
		size_t Num = 1;


		X_ASSERT(numAllocations_ == 0, "Memory leaks detected.")(numAllocations_);

		if (!gEnv || !gEnv->pLog) 
		{
			// allow stepping over.

			X_DISABLE_WARNING(4127)
			while (true)
			X_ENABLE_WARNING(4127)
			{
				if (it == end) {
					break;
				}

				const AllocationData& info = it->second; 

				X_UNUSED(info);
			}

			return;
		}

X_DISABLE_WARNING(4127)
		while (true)
X_ENABLE_WARNING(4127)
		{
			if (it == end)
				break;

			const AllocationData& info = it->second; // m_value;

			X_ERROR("ExMemTracking", "Unfreed allocation %d at address 0x%08p:", Num, it->first);

			{
				X_LOG_BULLET;

				X_LOG0("ExMemTracking", "Arena name: \"%s\"", info.memoryArenaName_);
#if X_ENABLE_MEMORY_HUMAN_IDS
				X_LOG0("ExMemTracking", "Allocation ID: \"%s\"", info.ID_);
				X_LOG0("ExMemTracking", "Type name: \"%s\"", info.typeName_);
#endif // !X_ENABLE_MEMORY_HUMAN_IDS
				X_LOG0("ExMemTracking", "Original size: %d", info.originalSize_);
				//	X_LOG0( "ExMemTracking", "Allocated size: %d", info.);

#if X_ENABLE_MEMORY_SOURCE_INFO
				const SourceInfo& sourceInfo = info.sourceInfo_;

				X_LOG0("ExMemTracking", "Filename(line): \"%s(%d)\"", sourceInfo.pFile_, sourceInfo.line_);
				X_LOG0("ExMemTracking", "Function: \"%s\"", sourceInfo.pFunction_);
				X_LOG0("ExMemTracking", "Function signature: \"%s\"", sourceInfo.pFunctionSignature_);
#endif // !X_ENABLE_MEMORY_SOURCE_INFO

				// call stack
				const CallStack& stack = info.callStack_;
				core::CallStack::Description Dsc;

				stack.ToDescription(Dsc);

				X_LOG0("ExMemTracking", "CallStack:\n%s", Dsc);
			
			}
			++it;
			Num++;
		}
	}
}


/// Stores the allocation along with additional data and a call stack in a hash map.
void FullMemoryTracking::OnAllocation(void* memory, size_t originalSize, size_t internalSize, 
	size_t alignment, size_t offset
	X_MEM_HUMAN_IDS_CB(const char* ID)
	X_MEM_HUMAN_IDS_CB(const char* typeName)
	X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), const char* memoryArenaName)
{
	X_UNUSED(alignment);
	X_UNUSED(offset);

	numAllocations_++;
	table_.insert(std::make_pair(memory, 
			AllocationData(
				originalSize, 
				internalSize
				X_MEM_IDS(ID, typeName)
				X_SOURCE_INFO_MEM_CB(sourceInfo),
				memoryArenaName,
				CallStack(1)
			)
		)
	);
}


/// Removes the allocation data from a hash map.
void FullMemoryTracking::OnDeallocation(void* memory)
{
	numAllocations_--;
	table_.erase(memory);
}




X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS
