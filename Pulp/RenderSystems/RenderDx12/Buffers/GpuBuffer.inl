

X_NAMESPACE_BEGIN(render)

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& GpuBuffer::getUAV(void) const
{
    return UAV_;
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& GpuBuffer::getSRV(void) const
{
    return SRV_;
}

X_INLINE D3D12_GPU_VIRTUAL_ADDRESS GpuBuffer::rootConstantBufferView(void) const
{
    return gpuVirtualAddress_;
}

X_INLINE D3D12_VERTEX_BUFFER_VIEW GpuBuffer::vertexBufferView(size_t baseVertexIndex) const
{
    const size_t offset = baseVertexIndex * elementSize_;
    return vertexBufferView(offset, safe_static_cast<uint32_t, size_t>(bufferSize_ - offset), elementSize_);
}

X_INLINE D3D12_INDEX_BUFFER_VIEW GpuBuffer::indexBufferView(size_t startIndex) const
{
    const size_t offset = startIndex * elementSize_;
    return indexBufferView(offset, safe_static_cast<uint32_t, size_t>(bufferSize_ - offset), elementSize_ == 4);
}

X_NAMESPACE_END