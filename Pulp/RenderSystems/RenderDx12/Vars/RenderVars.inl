

X_NAMESPACE_BEGIN(render)

X_INLINE bool RenderVars::varsRegisterd(void) const
{
    return varsRegisterd_ != 0;
}

X_INLINE bool RenderVars::enableDebugLayer(void) const
{
    return debugLayer_ != 0;
}

X_INLINE const Colorf& RenderVars::getClearCol(void) const
{
    return clearColor_;
}

X_NAMESPACE_END