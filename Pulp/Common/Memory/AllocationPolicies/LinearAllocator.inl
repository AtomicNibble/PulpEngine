
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void LinearAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void LinearAllocator::free(void* ptr, size_t size)
{
	X_ASSERT_NOT_NULL(ptr);
	X_UNUSED(size);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void LinearAllocator::reset(void)
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	statistics_.allocationCount_ = 0;
	statistics_.physicalMemoryUsed_ = 0;
	statistics_.wasteAlignment_ = 0;
	statistics_.internalOverhead_ = 0;
#endif

	current_ = start_;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t LinearAllocator::getSize(void* allocation) const
{
	union
	{
		void* as_void;
		char* as_char;
		size_t* as_size;
	};

	// grab the allocation's size from the first N bytes before the user data
	as_void = allocation;
	as_char -= sizeof(size_t);
	return (*as_size);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t LinearAllocator::usableSize(void* allocation) const
{
	return getSize(allocation);
}

