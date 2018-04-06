
X_NAMESPACE_BEGIN(render)

X_INLINE GpuResource::GpuResource() :
    GpuResource(nullptr, D3D12_RESOURCE_STATE_COMMON)

{
}

X_INLINE GpuResource::GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES currentState) :
    pResource_(pResource),
    usageState_(currentState),
    transitioningState_(static_cast<D3D12_RESOURCE_STATES>(-1)),
    gpuVirtualAddress_(static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(0))
{
}

X_INLINE GpuResource::~GpuResource()
{
    destroy();
}

X_INLINE void GpuResource::destroy(void)
{
    core::SafeReleaseDX(pResource_);
}

X_INLINE ID3D12Resource* GpuResource::operator->()
{
    return pResource_;
}

X_INLINE const ID3D12Resource* GpuResource::operator->() const
{
    return pResource_;
}

X_INLINE ID3D12Resource* GpuResource::getResource(void)
{
    return pResource_;
}

X_INLINE const ID3D12Resource* GpuResource::getResource(void) const
{
    return pResource_;
}

X_INLINE ID3D12Resource*& GpuResource::getResourcePtrRef(void)
{
    return pResource_;
}

X_INLINE D3D12_GPU_VIRTUAL_ADDRESS GpuResource::getGpuVirtualAddress(void) const
{
    return gpuVirtualAddress_;
}

X_INLINE D3D12_RESOURCE_STATES GpuResource::getUsageState(void) const
{
    return usageState_;
}

X_INLINE D3D12_RESOURCE_STATES GpuResource::getTransitioningStateState(void) const
{
    return transitioningState_;
}

X_INLINE void GpuResource::setUsageState(D3D12_RESOURCE_STATES state)
{
    usageState_ = state;
}

X_INLINE void GpuResource::setTransitioningStateState(D3D12_RESOURCE_STATES state)
{
    transitioningState_ = state;
}

X_INLINE void GpuResource::setGpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress)
{
    gpuVirtualAddress_ = gpuVirtualAddress;
}

X_NAMESPACE_END
