

X_INLINE xStopWatch::xStopWatch(void) :
start_(SysTimer::Get())
{
	
}


X_INLINE void xStopWatch::Start(void)
{
	start_ = SysTimer::Get();
}

X_INLINE uint64_t xStopWatch::GetCount(void) const
{
	return safe_static_cast<uint64_t>(SysTimer::Get() - start_);
}


X_INLINE float xStopWatch::GetSeconds(void) const
{
	return SysTimer::ToSeconds(GetCount());
}


X_INLINE float xStopWatch::GetMilliSeconds(void) const
{
	return SysTimer::ToMilliSeconds(GetCount());
}

