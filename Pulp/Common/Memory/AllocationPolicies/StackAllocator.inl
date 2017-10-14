

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t StackAllocator::getSize(void* allocation) const
{
	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	// grab the allocation's size from the first N bytes before the user data
	as_void = allocation;
	return as_header[-1].AllocationSize_;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t StackAllocator::usableSize(void* allocation) const
{
	return getSize(allocation);
}

