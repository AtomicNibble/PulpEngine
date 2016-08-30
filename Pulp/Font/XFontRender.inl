
X_NAMESPACE_BEGIN(font)


X_INLINE void XFontRender::SetSizeRatio(float fSizeRatio)
{ 
	fSizeRatio_ = fSizeRatio; 
}

X_INLINE float XFontRender::GetSizeRatio(void) const 
{ 
	return fSizeRatio_; 
}

X_INLINE FT_Encoding XFontRender::GetEncoding(void) const 
{ 
	return pEncoding_; 
}


X_NAMESPACE_END
