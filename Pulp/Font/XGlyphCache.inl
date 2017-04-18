

X_NAMESPACE_BEGIN(font)


X_INLINE bool XGlyphCache::IsLoaded(void) const
{
	return fontRenderer_.ValidFace();
}

X_INLINE bool XGlyphCache::SetEncoding(FontEncoding::Enum encoding)
{ 
	return fontRenderer_.SetEncoding(encoding);
}

X_INLINE FontEncoding::Enum XGlyphCache::GetEncoding(void) const
{
	return fontRenderer_.GetEncoding();
}



X_NAMESPACE_END
