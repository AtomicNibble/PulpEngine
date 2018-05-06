

X_NAMESPACE_BEGIN(net)

X_INLINE int32_t SessionVars::connectionAttemps(void) const
{
    return connectionAttemps_;
}

X_INLINE int32_t SessionVars::connectionRetryDelayMs(void) const
{
    return connectionRetyDelayMs_;
}


X_NAMESPACE_END
