

X_NAMESPACE_BEGIN(net)

X_INLINE bool SessionVars::lobbyDebug(void) const
{
    return lobbyDebug_ != 0;
}

X_INLINE int32_t SessionVars::drawLobbyDebug(void) const
{
    return drawLobbyDebug_;
}

X_INLINE int32_t SessionVars::connectionAttemps(void) const
{
    return connectionAttemps_;
}

X_INLINE int32_t SessionVars::connectionRetryDelayMs(void) const
{
    return connectionRetyDelayMs_;
}

X_INLINE bool SessionVars::snapDebug(void) const
{
    return snapDebug_ != 0;
}

X_INLINE bool SessionVars::snapFroceDrop(void)
{
    if (snapFroceDrop_ == 0) {
        return false;
    }

    snapFroceDrop_ = 0;
    return true;
}

X_INLINE int32_t SessionVars::snapMaxbufferedMs(void) const
{
    return snapMaxbufferedMs_;
}

X_INLINE int32_t SessionVars::snapRateMs(void) const
{
    return snapRateMs_;
}

X_INLINE int32_t SessionVars::userCmdRateMs(void) const
{
    return userCmdRateMs_;
}

X_INLINE int32_t SessionVars::waitForPlayers(void) const
{
    return waitForPlayers_;
}

X_NAMESPACE_END
