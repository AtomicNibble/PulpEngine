

X_NAMESPACE_BEGIN(render)



X_INLINE const ByteAddressBuffer& X3DBuffer::getBuf(void) const
{
	X_ASSERT_NOT_NULL(pBuffer_);
	return *pBuffer_;
}


X_INLINE ByteAddressBuffer& X3DBuffer::getBuf(void)
{
	X_ASSERT_NOT_NULL(pBuffer_);
	return *pBuffer_;
}

X_INLINE BufUsage::Enum X3DBuffer::getUsage(void) const
{
	return usage_;
}

X_INLINE uint32_t X3DBuffer::getSize(void) const
{
	return size_;
}

// ------------------------------------------



// ------------------------------------------


X_INLINE BufferManager::BufferHandle BufferManager::createHandleForBuffer(X3DBuffer* pBuf)
{
	return reinterpret_cast<BufferManager::BufferHandle>(pBuf);
}

X_INLINE X3DBuffer* BufferManager::bufferForHandle(BufferHandle handle)
{
	return reinterpret_cast<X3DBuffer*>(handle);
}

X_INLINE ConstBuffer* BufferManager::constBufferForHandle(BufferHandle handle)
{
	return reinterpret_cast<ConstBuffer*>(handle);
}

// ------------------------------------------



X_INLINE X3DBuffer* BufferManager::IBFromHandle(IndexBufferHandle bufHandle)
{
	return bufferForHandle(bufHandle);
}

X_INLINE X3DBuffer* BufferManager::VBFromHandle(VertexBufferHandle bufHandle)
{
	return bufferForHandle(bufHandle);
}

X_INLINE ConstBuffer* BufferManager::CBFromHandle(ConstantBufferHandle bufHandle)
{
	return constBufferForHandle(bufHandle);
}


X_NAMESPACE_END
