
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void SimpleMemoryTracking::OnAllocation(void*, size_t, size_t, size_t, size_t
	X_MEM_HUMAN_IDS_CB(const char*)
	X_MEM_HUMAN_IDS_CB(const char*)
	X_SOURCE_INFO_MEM_CB(const SourceInfo&), const char*)
{
	++numAllocations_;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void SimpleMemoryTracking::OnDeallocation(void*)
{
	--numAllocations_;
}
