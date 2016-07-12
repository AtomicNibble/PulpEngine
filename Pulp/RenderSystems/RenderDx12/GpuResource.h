#pragma once


X_NAMESPACE_BEGIN(render)


class GpuResource
{
public:
	X_INLINE GpuResource();
	X_INLINE GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES currentState);
	X_INLINE ~GpuResource();

	X_INLINE void destroy(void);

	X_INLINE ID3D12Resource* operator->();
	X_INLINE const ID3D12Resource* operator->() const;

	X_INLINE ID3D12Resource* getResource(void);
	X_INLINE const ID3D12Resource* getResource(void) const;

	X_INLINE D3D12_GPU_VIRTUAL_ADDRESS getGpuVirtualAddress(void) const;

protected:
	ID3D12Resource* pResource_;
	D3D12_RESOURCE_STATES usageState_;
	D3D12_RESOURCE_STATES transitioningState_;
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress_;
};




X_NAMESPACE_END

#include "GpuResource.inl"