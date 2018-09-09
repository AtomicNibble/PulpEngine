
X_NAMESPACE_BEGIN(video)

X_INLINE bool VideoVars::drawDebug(void) const
{
    return drawDebug_ != 0;
}

X_INLINE int32_t VideoVars::debug(void) const
{
    return debug_;
}

X_NAMESPACE_END