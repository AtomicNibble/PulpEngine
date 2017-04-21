
X_NAMESPACE_BEGIN(font)

X_INLINE bool XFontRender::ValidFace(void) const
{
	return pFace_ != nullptr;
}

X_INLINE void XFontRender::EnabledDebugRender(bool enable)
{
	debugRender_ = enable;
}

X_INLINE void XFontRender::SetSizeRatio(float sizeRatio)
{ 
	sizeRatio_ = sizeRatio;
}

X_INLINE float XFontRender::GetSizeRatio(void) const 
{ 
	return sizeRatio_;
}

X_INLINE FontEncoding::Enum XFontRender::GetEncoding(void) const
{ 
	return encoding_;
}

X_INLINE const Metrics& XFontRender::GetMetrics(void) const
{
	return metrics_;
}


X_NAMESPACE_END
