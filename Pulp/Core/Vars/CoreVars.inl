
X_NAMESPACE_BEGIN(core)

X_INLINE int32_t CoreVars::getFullscreen(void) const
{
    return fullscreen_;
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