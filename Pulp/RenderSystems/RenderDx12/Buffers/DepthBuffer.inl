
X_NAMESPACE_BEGIN(render)

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& DepthBuffer::getDSV(void) const
{ 
	return hDSV_[0];
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& DepthBuffer::getDSV_DepthReadOnly(void) const
{ 
	return hDSV_[1];
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& DepthBuffer::getDSV_StencilReadOnly(void) const
{
	return hDSV_[2];
}

X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& DepthBuffer::getDSV_ReadOnly(void) const
{ 
	return hDSV_[3];
}


X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& DepthBuffer::getStencilSRV(void) const
{ 
	return hStencilSRV_;
}


X_INLINE float32_t DepthBuffer::getClearDepth(void) const 
{ 
	return clearDepth_;
}

X_INLINE uint32_t DepthBuffer::getClearStencil(void) const 
{ 
	return clearStencil_;
}


X_NAMESPACE_END