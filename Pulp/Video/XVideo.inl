
X_NAMESPACE_BEGIN(video)

X_INLINE VideoState::Enum Video::getState(void) const
{
    return state_;
}

X_INLINE uint16_t Video::getWidth(void) const
{
    return video_.pixelWidth;
}

X_INLINE uint16_t Video::getHeight(void) const
{
    return video_.pixelHeight;
}

X_INLINE uint32_t Video::getFps(void) const
{
    return frameRate_;
}

X_INLINE uint32_t Video::getNumFrames(void) const
{
    return video_.numFrames;
}

X_INLINE uint32_t Video::getCurFrame(void) const
{
    return curFrame_;
}

X_INLINE size_t Video::getIOBufferSize(void) const
{
    return ioRingBuffer_.size();
}

X_INLINE bool Video::hasFrame(void) const
{
    return presentFrame_;
}

X_NAMESPACE_END