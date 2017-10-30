
X_NAMESPACE_BEGIN(video)

X_INLINE uint16_t Video::getWidth(void) const
{
	return width_;
}

X_INLINE uint16_t Video::getHeight(void) const
{
	return height_;
}

X_INLINE uint32_t Video::getFps(void) const
{
	return frameRate_;
}

X_INLINE uint32_t Video::getNumFrames(void) const
{
	return numFrames_;
}


X_NAMESPACE_END