
X_NAMESPACE_BEGIN(render)


// Get pre-created CPU-visible descriptor handles
X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& ColorBuffer::getSRV(void) const
{
	return SRVHandle_;
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& ColorBuffer::getRTV(void) const
{
	return RTVHandle_;

}
X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& ColorBuffer::getUAV(void) const
{
	return UAVHandle_[0];
}

X_INLINE Colorf ColorBuffer::getClearColor(void) const
{
	return clearColor_;

}

X_NAMESPACE_END