#pragma once


X_NAMESPACE_BEGIN(render)

X_INLINE bool CommandQue::IsReady(void) const
{
	return pCommandQueue_ != nullptr;
}

X_INLINE ID3D12CommandQueue* CommandQue::getCommandQueue(void)
{
	return pCommandQueue_;
}

// -------------------------------------------

X_INLINE CommandQue& CommandListManger::getGraphicsQueue(void)
{
	return graphicsQueue_;
}

X_INLINE CommandQue& CommandListManger::getComputeQueue(void)
{
	return computeQueue_;
}

X_INLINE CommandQue& CommandListManger::getCopyQueue(void)
{
	return copyQueue_;
}

X_INLINE CommandQue& CommandListManger::getQueue(D3D12_COMMAND_LIST_TYPE Type)
{
	switch (Type)
	{
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return computeQueue_;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return copyQueue_;
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return graphicsQueue_;
#if X_DEBUG
	default:
		X_ASSERT_UNREACHABLE();
		return graphicsQueue_;
#else
		X_NO_SWITCH_DEFAULT;
#endif 
	}

	return graphicsQueue_;

}

X_NAMESPACE_END
