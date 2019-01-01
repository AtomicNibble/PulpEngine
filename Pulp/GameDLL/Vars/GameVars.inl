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

X_INLINE int32_t GameVars::chatMsgLifeMS(void) const
{
    return chatLifeMS_;
}

X_INLINE int32_t GameVars::drawGameUserDebug(void) const
{
    return drawGameUserDebug_;
}

X_INLINE int32_t GameVars::drawSessionInfoDebug(void) const
{
    return drawSessionInfoDebug_;
}

X_INLINE int32_t GameVars::drawBulletRay(void) const
{
    return drawBulletRay_;
}

X_INLINE core::ICVar* GameVars::getFovVar(void) const
{
    return pFovVar_;
}

X_NAMESPACE_END
