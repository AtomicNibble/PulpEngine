

X_NAMESPACE_BEGIN(net)


X_INLINE void ReliabilityLayer::setTimeout(core::TimeVal timeout)
{
	timeOut_ = timeout;
}

X_INLINE core::TimeVal ReliabilityLayer::getTimeout(void)
{
	return timeOut_;
}

X_INLINE void ReliabilityLayer::setUnreliableMsgTimeout(core::TimeVal timeout)
{
	unreliableTimeOut_ = timeout;
}

X_INLINE core::TimeVal ReliabilityLayer::getUnreliableMsgTimeout(void)
{
	return unreliableTimeOut_;
}


X_NAMESPACE_END