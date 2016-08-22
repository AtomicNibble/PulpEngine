

X_NAMESPACE_BEGIN(render)



X_INLINE const ByteAddressBuffer& X3DBuffer::getBuf(void) const
{
	X_ASSERT_NOT_NULL(pBuffer_);
	return *pBuffer_;
}

// ------------------------------------------


X_INLINE X3DBuffer* BufferManager::IBFromHandle(IndexBufferHandle bufHandle) const
{
	return bufferForHandle(bufHandle);
}

X_INLINE X3DBuffer* BufferManager::VBFromHandle(VertexBufferHandle bufHandle) const
{
	return bufferForHandle(bufHandle);
}


X_NAMESPACE_END
