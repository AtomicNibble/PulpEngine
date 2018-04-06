

X_INLINE StopWatch::StopWatch(void) :
    start_(SysGet())
{
}

X_INLINE void StopWatch::Start(void)
{
    start_ = SysGet();
}

X_INLINE int64_t StopWatch::GetCount(void) const
{
    return safe_static_cast<uint64_t>(SysGet() - start_);
}

X_INLINE float StopWatch::GetSeconds(void) const
{
    return SysToSeconds(GetCount());
}

X_INLINE float StopWatch::GetMilliSeconds(void) const
{
    return SysToMilliSeconds(GetCount());
}

X_INLINE TimeVal StopWatch::GetTimeVal(void) const
{
    int64_t time = GetCount();
    return TimeVal(time);
}

X_INLINE TimeVal StopWatch::GetStart(void) const
{
    return TimeVal(start_);
}

X_INLINE TimeVal StopWatch::GetEnd(void) const
{
    return TimeVal(SysGet());
}