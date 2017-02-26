
X_NAMESPACE_BEGIN(net)

X_INLINE bool PingAndClockDifferential::isValid(void) const
{
	return pingTime != UNDEFINED_PING;
}

X_NAMESPACE_END
