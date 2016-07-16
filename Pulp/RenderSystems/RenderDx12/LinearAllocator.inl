

X_NAMESPACE_BEGIN(render)

void DynAlloc::setData(void* pData, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
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




X_NAMESPACE_END
