

X_NAMESPACE_BEGIN(font)




X_INLINE uint8_t* XGlyphBitmap::GetBuffer(void)
{ 
	return pBuffer_; 
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
