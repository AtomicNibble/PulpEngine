

X_NAMESPACE_BEGIN(render)

X_INLINE bool RenderVars::drawAux(void) const
{
	return drawAux_ != 0;
}

X_INLINE const Colorf& RenderVars::getClearCol(void) const
{
	return r_clear_color;
}

X_NAMESPACE_END