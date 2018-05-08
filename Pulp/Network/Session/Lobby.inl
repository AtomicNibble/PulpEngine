
X_NAMESPACE_BEGIN(net)

X_INLINE LobbyState::Enum Lobby::getState(void) const
{
    return state_;
}

X_INLINE LobbyType::Enum Lobby::getType(void) const
{
    return type_;
}

X_INLINE bool Lobby::isHost(void) const
{
    return isHost_;
}

X_INLINE bool Lobby::isPeer(void) const
{
    // you are only a peer if connected to a host.

    return !isHost();
}

X_INLINE bool Lobby::hasFinishedLoading(void) const
{
    return finishedLoading_;
}

X_INLINE MatchFlags Lobby::getMatchFlags(void) const
{
    return params_.flags;
}

X_INLINE const MatchParameters& Lobby::getMatchParams(void) const
{
    return params_;
}

X_INLINE int32_t Lobby::getNumFreeSlots(void) const
{
    return params_.numSlots - getNumUsers();
}

X_INLINE bool Lobby::isFull(void) const
{
    return getNumFreeSlots() == 0;
}


X_NAMESPACE_END
