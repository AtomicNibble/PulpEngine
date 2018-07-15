
X_NAMESPACE_BEGIN(net)

X_INLINE int32_t Lobby::getHostPeerIdx(void) const
{
    return hostIdx_;
}

// ---------------------------------

X_INLINE int32_t Lobby::getNumUsers(void) const
{
    return safe_static_cast<int32_t>(users_.size());
}

X_INLINE int32_t Lobby::getNumFreeUserSlots(void) const
{
    return params_.numSlots - getNumUsers();
}

X_INLINE LobbyUserHandle Lobby::getUserHandleForIdx(int32_t idx) const
{
    return static_cast<LobbyUserHandle>(idx);
}

// ---------------------------------

X_INLINE bool Lobby::isActive(void) const
{
    return isHost() || isPeer();
}

X_INLINE bool Lobby::isHost(void) const
{
    return isHost_;
}

X_INLINE bool Lobby::isPeer(void) const
{
    if (hostIdx_ < 0) {
        return false;
    }

    X_ASSERT(!isHost(), "Can't host a lobby if peer of another")(isHost(), hostIdx_);
    return true;
}

X_INLINE LobbyType::Enum Lobby::getType(void) const
{
    return type_;
}

X_INLINE const MatchParameters& Lobby::getMatchParams(void) const
{
    return params_;
}

X_INLINE MatchFlags Lobby::getMatchFlags(void) const
{
    return getMatchParams().flags;
}

// ---------------------------------

X_INLINE LobbyState::Enum Lobby::getState(void) const
{
    return state_;
}

X_INLINE bool Lobby::shouldStartLoading(void) const
{
    return startLoading_;
}

X_INLINE void Lobby::beganLoading(void)
{
    X_ASSERT(startLoading_, "beganLoading called when startLoading_ is false")(startLoading_);
    startLoading_ = false;
}

X_INLINE bool Lobby::hasFinishedLoading(void) const
{
    return finishedLoading_;
}





X_NAMESPACE_END
