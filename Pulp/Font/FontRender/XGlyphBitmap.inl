

X_NAMESPACE_BEGIN(font)




X_INLINE typename XGlyphBitmap::DataVec& XGlyphBitmap::GetBuffer(void)
{ 
	return buffer_; 
}

X_INLINE typename const XGlyphBitmap::DataVec& XGlyphBitmap::GetBuffer(void) const
{
	return buffer_;
}


X_INLINE int32_t XGlyphBitmap::GetWidth(void) const 
{ 
	return width_; 
}

X_INLINE int32_t XGlyphBitmap::GetHeight(void) const
{ 
	return height_; 
}



X_NAMESPACE_END
