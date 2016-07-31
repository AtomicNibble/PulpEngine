

X_NAMESPACE_BEGIN(render)

X_INLINE bool RenderVars::drawAux(void) const
{
	return drawAux_ != 0;
}

X_INLINE const Colorf& RenderVars::getClearCol(void) const
{
	return clearColor_;
}


X_NAMESPACE_END