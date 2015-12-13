

X_INLINE StopWatch::StopWatch(void) :
start_(SysTimer::Get())
{
	
}


X_INLINE void StopWatch::Start(void)
{
	start_ = SysTimer::Get();
}

X_INLINE uint64_t StopWatch::GetCount(void) const
{
	return safe_static_cast<uint64_t>(SysTimer::Get() - start_);
}


X_INLINE float StopWatch::GetSeconds(void) const
{
	return SysTimer::ToSeconds(GetCount());
}


X_INLINE float StopWatch::GetMilliSeconds(void) const
{
	return SysTimer::ToMilliSeconds(GetCount());
}

