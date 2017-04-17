

X_NAMESPACE_BEGIN(font)

X_INLINE int32_t FontVars::glyphCacheSize(void) const
{
	return glyphCacheSize_;
}

X_INLINE bool FontVars::glyphCachePreWarm(void) const
{
	return glyphCachePreWarm_ != 0;
}


X_NAMESPACE_END