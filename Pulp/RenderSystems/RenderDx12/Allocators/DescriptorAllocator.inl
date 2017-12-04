#pragma once


X_NAMESPACE_BEGIN(render)


X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num)
{
	return allocators_[type].allocate(num);
}

X_NAMESPACE_END