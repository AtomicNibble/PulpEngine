

// ---------------------------------------------------------------------------------------------------------------------
inline void* GrowingPoolAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
	return allocate(size, alignment, offset, nullptr, 0);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class T>
inline void* GrowingPoolAllocator::allocate(size_t size, size_t alignment, size_t offset, 
	typename std::enable_if<!core::compileTime::IsPointer<T>::Value, T>::type& chunkHeader)
{
	X_ASSERT(chunkHeaderSize_ == sizeof(chunkHeader), "Given chunk header does not match the size given upon initialization.")(chunkHeaderSize_, sizeof(chunkHeader));
	return allocate(size, alignment, offset, &chunkHeader, sizeof(chunkHeader));
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void GrowingPoolAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	freelist_.Return(ptr);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	statistics_.allocationCount_--;
	statistics_.physicalMemoryUsed_ -= elementSize_;
	statistics_.wasteAlignment_ -= wastePerElement_;
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void GrowingPoolAllocator::free(void* ptr, size_t size)
{
	X_UNUSED(size);

	free(ptr);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t GrowingPoolAllocator::getSize(void*) const
{
	return maxSize_;	
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t GrowingPoolAllocator::usableSize(void*) const
{
	return maxSize_;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline bool GrowingPoolAllocator::containsAllocation(void* ptr) const
{
	return ((ptr >= virtualStart_) && (ptr < physicalCurrent_));
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline T* GrowingPoolAllocator::getChunkHeader(void* allocationAddress, unsigned int chunkSize, unsigned int chunkHeaderSize)
{
	// all allocations lie in one of the free lists which are always allocated in one of the physical memory chunks.
	// by rounding the allocation address to the next multiple of the chunk size, we are guaranteed to be at the end
	// of one of the chunks. because headers always lie at the end of a chunk, simply subtracting the chunk header size
	// points us right to the start of the header.
	union
	{
		void* as_void;
		char* as_char;
		T* as_T;
	};

	as_void = allocationAddress;
	as_char = pointerUtil::AlignTop(as_char + chunkHeaderSize, chunkSize) - chunkHeaderSize;
	return as_T;
}
