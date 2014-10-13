

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t StackAllocator::getSize(void* allocation) const
{
	union
	{
		void* as_void;
		char* as_char;
		size_t* as_size_t;
	};

	// grab the allocation's size from the first N bytes before the user data
	as_void = allocation;
	as_char -= sizeof(size_t);
	return (*as_size_t);
}
