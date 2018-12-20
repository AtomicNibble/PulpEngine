
X_NAMESPACE_BEGIN(net)

X_INLINE const SnapShot::PlayerTimeMSArr& SnapShot::getUserCmdTimes(void) const
{
    return userCmdTimes_;
}

X_INLINE void SnapShot::setPlayerGuids(const PlayerGuidArr& userGuids)
{
    userGuids_ = userGuids;
}

X_INLINE const SnapShot::PlayerGuidArr& SnapShot::getPlayerGuids(void) const
{
    return userGuids_;
}

X_INLINE size_t SnapShot::getNumObjects(void) const
{
    return objs_.size();
}

X_INLINE int32_t SnapShot::getTimeMS(void) const
{
    return timeMS_;
}

X_INLINE int32_t SnapShot::getRecvTimeMS(void) const
{
    return recvTimeMS_;
}

X_INLINE void SnapShot::setTime(int32_t time)
{
    timeMS_ = time;
}

X_INLINE void SnapShot::setRecvTime(int32_t time)
{
    recvTimeMS_ = time;
}

X_NAMESPACE_END
