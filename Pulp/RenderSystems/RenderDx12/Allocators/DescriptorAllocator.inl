#pragma once


X_NAMESPACE_BEGIN(render)


X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num)
{
	return allocators_[type].allocate(num);
}

// ----------------------------------------------------


X_INLINE DescriptorHandle::DescriptorHandle()
{
	cpuHandle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	gpuHandle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

X_INLINE DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) :
	cpuHandle_(cpuHandle)
{
	gpuHandle_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

X_INLINE DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, 
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) :
	cpuHandle_(cpuHandle), 
	gpuHandle_(gpuHandle)
{
}

X_INLINE DescriptorHandle DescriptorHandle::operator+ (int32_t offsetScaledByDescriptorSize) const
{
	DescriptorHandle ret = *this;
	ret += offsetScaledByDescriptorSize;
	return ret;
}

X_INLINE void DescriptorHandle::operator += (int32_t offsetScaledByDescriptorSize)
{
	if (cpuHandle_.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		cpuHandle_.ptr += offsetScaledByDescriptorSize;
	}
	if (gpuHandle_.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		gpuHandle_.ptr += offsetScaledByDescriptorSize;
	}
}

X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::getCpuHandle(void) const 
{ 
	return cpuHandle_;
}
X_INLINE D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::getGpuHandle(void) const 
{ 
	return gpuHandle_;
}

X_INLINE bool DescriptorHandle::isNull(void) const 
{ 
	return cpuHandle_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

X_INLINE bool DescriptorHandle::isShaderVisible(void) const 
{
	return gpuHandle_.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}



X_NAMESPACE_END