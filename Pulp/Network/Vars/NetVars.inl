

X_NAMESPACE_BEGIN(net)


X_INLINE int32_t NetVars::debugEnabled(void) const
{
	return debug_;
}

X_INLINE int32_t NetVars::debugSocketsEnabled(void) const
{
	return debugSockets_;
}


X_INLINE bool NetVars::rlConnectionsPerIP(void) const
{
	return rlconnectionsPerIp_ != 0;
}

X_INLINE int32_t NetVars::rlConnectionsPerIPThreshMS(void) const
{
	return rlconnectionsPerIpThreshMS_;
}

X_INLINE int32_t NetVars::rlConnectionsPerIPBanTimeMS(void) const
{
	return rlconnectionsPerIpBanTimeMS_;
}

X_INLINE int32_t NetVars::dropPartialConnectionsMS(void) const
{
	return dropPartialConnectionsMS_;
}

X_INLINE int32_t NetVars::pingTimeMS(void) const
{
	return pingTimeMS_;
}

X_INLINE int32_t NetVars::unreliableTimeoutMS(void) const
{
	return unreliableTimeoutMS_;
}

X_INLINE int32_t NetVars::connectionBSPLimit(void) const
{
	return connectionBSPLimit_;
}

X_NAMESPACE_END