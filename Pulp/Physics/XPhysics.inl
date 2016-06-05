

X_NAMESPACE_BEGIN(physics)


X_INLINE bool XPhysics::IsPaused(void) const
{
	return pause_;
}

X_INLINE void XPhysics::togglePause(void)
{
	pause_ = !pause_;
}

X_NAMESPACE_END