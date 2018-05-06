

X_NAMESPACE_BEGIN(net)

X_INLINE int32_t SessionVars::connectionAttemps(void) const
{
    return connectionAttemps_;
}

X_INLINE int32_t SessionVars::connectionRetryDelayMs(void) const
{
    return connectionRetyDelayMs_;
}

X_INLINE bool SessionVars::snapFroceDrop(void)
{
    if (snapFroceDrop_ == 0) {
        return false;
    }

    snapFroceDrop_ = 0;
    return true;
}

X_NAMESPACE_END
