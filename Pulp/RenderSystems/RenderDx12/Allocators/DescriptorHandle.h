#pragma once

X_NAMESPACE_BEGIN(render)

class DescriptorHandle
{
public:
	X_INLINE DescriptorHandle();
	X_INLINE explicit DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle);
	X_INLINE DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle);

	X_INLINE DescriptorHandle operator+ (int32_t offsetScaledByDescriptorSize) const;

	X_INLINE void operator += (int32_t offsetScaledByDescriptorSize);

	X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(void) const;
	X_INLINE D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(void) const;

	X_INLINE bool isNull(void) const;
	X_INLINE bool isShaderVisible(void) const;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_;
};


X_NAMESPACE_END

#include "DescriptorHandle.inl"