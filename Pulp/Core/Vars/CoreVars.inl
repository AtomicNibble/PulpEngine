
X_NAMESPACE_BEGIN(core)

X_INLINE int32_t CoreVars::getSchedulerNumThreads(void) const
{
    return schedulerNumThreads_;
}

X_INLINE int32_t CoreVars::getCoreEventDebug(void) const
{
    return coreEventDebug_;
}

X_INLINE int32_t CoreVars::getCoreFastShutdown(void) const
{
    return coreFastShutdown_;
}

X_INLINE int32_t CoreVars::getWinPosX(void) const
{
    return winXPos_;
}

X_INLINE int32_t CoreVars::getWinPosY(void) const
{
    return winYPos_;
}

X_INLINE int32_t CoreVars::getWinWidth(void) const
{
    return winWidth_;
}

X_INLINE int32_t CoreVars::getWinHeight(void) const
{
    return winHeight_;
}

X_INLINE int32_t CoreVars::getFullscreen(void) const
{
    return fullscreen_;
}

X_INLINE int32_t CoreVars::getMonitor(void) const
{
    return monitor_;
}

X_INLINE int32_t CoreVars::getWinVideoMode(void) const
{
    return videoMode_;
}

X_INLINE core::ICVar* CoreVars::getVarWinPosX(void)
{
    return pWinPosX_;
}

X_INLINE core::ICVar* CoreVars::getVarWinPosY(void)
{
    return pWinPosY_;
}

X_INLINE core::ICVar* CoreVars::getVarWinWidth(void)
{
    return pWinWidth_;
}

X_INLINE core::ICVar* CoreVars::getVarWinHeight(void)
{
    return pWinHeight_;
}

X_INLINE core::ICVar* CoreVars::getVarWinCustomFrame(void)
{
    return pWinCustomFrame_;
}

X_NAMESPACE_END