

X_NAMESPACE_BEGIN(render)

X_INLINE void DynAlloc::setData(void* pData, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	pData_ = pData;
	gpuAddress_ = gpuAddress;
}


X_INLINE size_t DynAlloc::getOffset(void) const
{
	return offset_;
}

X_INLINE size_t DynAlloc::getSize(void) const
{
	return size_;
}

X_INLINE void* DynAlloc::getCpuData(void) const
{
	return pData_;
}

X_INLINE const GpuResource& DynAlloc::getBuffer(void) const
{
	return buffer_;
}

X_INLINE GpuResource& DynAlloc::getBuffer(void)
{
	return buffer_;
}

X_INLINE D3D12_GPU_VIRTUAL_ADDRESS DynAlloc::getGpuAddress(void)
{
	return gpuAddress_;
}


// ------------------------------------------------------------------

X_INLINE void* LinearAllocationPage::cpuVirtualAddress(void) const
{
	return pCpuVirtualAddress_;
}

X_INLINE D3D12_GPU_VIRTUAL_ADDRESS LinearAllocationPage::gpuVirtualAddress(void) const
{
	return gpuVirtualAddress_;
}

// ------------------------------------------------------------------

X_INLINE LinearAllocationPage* LinearAllocatorManager::requestPage(LinearAllocatorType::Enum type)
{
	return pageAllocators_[type].requestPage();
}

X_INLINE void LinearAllocatorManager::discardPages(LinearAllocatorType::Enum type, uint64_t fenceID, const core::Array<LinearAllocationPage*>& pages)
{
	return pageAllocators_[type].discardPages(fenceID, pages);
}


X_NAMESPACE_END
