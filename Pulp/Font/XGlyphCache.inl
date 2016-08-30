

X_NAMESPACE_BEGIN(font)


X_INLINE int XGlyphCache::SetEncoding(FT_Encoding pEncoding)
{ 
	return fontRenderer_.SetEncoding(pEncoding);
}

X_INLINE FT_Encoding XGlyphCache::GetEncoding(void) const
{
	return fontRenderer_.GetEncoding();
}

X_NAMESPACE_END
