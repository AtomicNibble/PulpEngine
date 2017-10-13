#pragma once

#ifndef X_NOMEMORYTRACKING__H
#define X_NOMEMORYTRACKING__H


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements a tracking policy for memory arenas.
/// \details This class implements the concepts of a tracking policy as expected by the MemoryArena class. It is
/// a no-op class, only containing empty implementations.
/// \sa SimpleMemoryTracking ExtendedMemoryTracking FullMemoryTracking
class NoMemoryTracking
{
public:
	/// A human-readable string literal containing the policy's type.
	static const char* const TYPE_NAME;

	/// Defines the amount of overhead that each allocation causes.
	static const size_t PER_ALLOCATION_OVERHEAD = 0;

	/// Empty implementation.
	inline void OnAllocation(void*, size_t, size_t, size_t, size_t
		X_MEM_HUMAN_IDS_CB(const char*)
		X_MEM_HUMAN_IDS_CB(const char*)
		X_SOURCE_INFO_MEM_CB(const SourceInfo&), const char*) const {}

	/// Empty implementation.
	inline void OnDeallocation(void*) const {}
};





X_NAMESPACE_END


#endif // X_NOMEMORYTRACKING__H
