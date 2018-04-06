
X_NAMESPACE_BEGIN(render)

// Get pre-created CPU-visible descriptor handles
X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& ColorBuffer::getRTV(void) const
{
    return RTVHandle_;
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& ColorBuffer::getUAV(void) const
{
    return UAVHandles_[0];
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE* ColorBuffer::getUAVs(void) const
{
    return UAVHandles_.begin();
}

X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE* ColorBuffer::getUAVs(void)
{
    return UAVHandles_.begin();
}

X_INLINE void ColorBuffer::setClearColor(const Colorf& col)
{
    clearColor_ = col;
}

X_INLINE Colorf ColorBuffer::getClearColor(void) const
{
    return clearColor_;
}

X_NAMESPACE_END