
X_NAMESPACE_BEGIN(game)


X_INLINE bool InputVars::toggleRun(void) const
{
    return toggleRun_ != 0;
}

X_INLINE bool InputVars::toggleCrouch(void) const
{
    return toggleCrouch_ != 0;
}

X_INLINE bool InputVars::toggleZoom(void) const
{
    return toggleZoom_ != 0;
}

X_NAMESPACE_END
