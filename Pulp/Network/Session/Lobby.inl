
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

X_INLINE int32_t Lobby::numUsers(void) const
{
    return static_cast<int32_t>(users_.size());
}

X_INLINE int32_t Lobby::numFreeSlots(void) const
{
    return params_.numSlots - numUsers();
}

X_INLINE bool Lobby::isFull(void) const
{
    return numFreeSlots() == 0;
}


X_NAMESPACE_END
