

X_NAMESPACE_BEGIN(render)

X_INLINE void DynAlloc::setData(void* pData, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	pData_ = pData;
	gpuAddress_ = gpuAddress;
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
