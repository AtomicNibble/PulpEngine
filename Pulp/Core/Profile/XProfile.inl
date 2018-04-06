
#if X_ENABLE_PROFILER

X_NAMESPACE_BEGIN(core)

namespace profiler
{
    X_INLINE const ProfilerVars& XProfileSys::getVars(void) const
    {
        return vars_;
    }

} // namespace profiler

X_NAMESPACE_END

#endif // !X_ENABLE_PROFILER