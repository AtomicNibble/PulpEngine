
X_NAMESPACE_BEGIN(render)



X_INLINE uint32_t PixelBuffer::getWidth(void) const
{
	return width_;
}

X_INLINE uint32_t PixelBuffer::getHeight(void) const
{
	return height_;
}

X_INLINE uint32_t PixelBuffer::getDepth(void) const
{
	return arraySize_;
}

X_INLINE const DXGI_FORMAT PixelBuffer::getFormat(void) const
{
	return format_;
}


X_NAMESPACE_END