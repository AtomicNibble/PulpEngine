
X_NAMESPACE_BEGIN(font)

X_INLINE bool XFontRender::ValidFace(void) const
{
	return pFace_ != nullptr;
}

X_INLINE void XFontRender::SetSizeRatio(float fSizeRatio)
{ 
	fSizeRatio_ = fSizeRatio; 
}

X_INLINE float XFontRender::GetSizeRatio(void) const 
{ 
	return fSizeRatio_; 
}

X_INLINE FontEncoding::Enum XFontRender::GetEncoding(void) const
{ 
	return encoding_;
}


X_NAMESPACE_END
