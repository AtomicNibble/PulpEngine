X_NAMESPACE_BEGIN(game)


X_INLINE int32_t GameVars::userCmdDebug(void) const
{
    return userCmdDebug_;
}

X_INLINE bool GameVars::userCmdDrawDebug(void) const
{
    return userCmdDrawDebug_ != 0;
}

X_INLINE int32_t GameVars::userCmdClientReplay(void) const
{
    return userCmdClientReplay_;
}

X_INLINE core::ICVar* GameVars::getFovVar(void) const
{
    return pFovVar_;
}

X_NAMESPACE_END
