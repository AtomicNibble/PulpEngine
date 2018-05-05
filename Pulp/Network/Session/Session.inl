
X_NAMESPACE_BEGIN(net)

X_INLINE IPeer* Session::getPeer(void) const
{
    return pPeer_;
}

X_INLINE SessionState::Enum Session::getState(void) const
{
    return state_;
}

X_NAMESPACE_END
