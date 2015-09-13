

// ---------------------------------------------------------------------------------------------------------------------
inline void* GrowingPoolAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
	return allocate(size, alignment, offset, nullptr, 0);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class T>
inline void* GrowingPoolAllocator::allocate(size_t size, size_t alignment, size_t offset, const T& chunkHeader)
{
	X_ASSERT(m_chunkHeaderSize == sizeof(chunkHeader), "Given chunk header does not match the size given upon initialization.")(m_chunkHeaderSize, sizeof(chunkHeader));
	return allocate(size, alignment, offset, chunkHeader, sizeof(chunkHeader));
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void GrowingPoolAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	m_freelist.Return(ptr);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	statistics_.allocationCount_--;
	statistics_.physicalMemoryUsed_ -= m_elementSize;
	statistics_.wasteAlignment_ -= m_wastePerElement;
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t GrowingPoolAllocator::getSize(void*) const
{
	return m_maxSize;	
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline bool GrowingPoolAllocator::containsAllocation(void* ptr) const
{
	return ((ptr >= m_virtualStart) && (ptr < m_physicalCurrent));
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
