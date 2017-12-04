

X_NAMESPACE_BEGIN(render)

X_INLINE ID3D12CommandQueue* CommandQue::getCommandQueue(void)
{
	return pCommandQueue_;
}

X_INLINE uint64_t CommandQue::getNextFenceValue(void) const
{
	return nextFenceValue_;
}

X_INLINE void CommandQue::waitForIdle(void)
{
	waitForFence(nextFenceValue_ - 1);
}



X_NAMESPACE_END