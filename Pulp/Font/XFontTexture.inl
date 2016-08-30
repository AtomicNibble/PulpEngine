
X_NAMESPACE_BEGIN(font)

X_INLINE const Vec2i XFontTexture::GetSize(void) const 
{ 
	return Vec2i(width_, height_);
}

X_INLINE const int XFontTexture::GetWidth(void) const
{ 
	return width_;
}

X_INLINE const int XFontTexture::GetHeight(void) const
{ 
	return height_; 
}

X_INLINE uint8* XFontTexture::GetBuffer(void) 
{ 
	return pBuffer_; 
}

X_INLINE float XFontTexture::GetTextureCellWidth(void) const
{ 
	return fTextureCellWidth_;
}

X_INLINE float XFontTexture::GetTextureCellHeight(void) const 
{ 
	return fTextureCellHeight_;
}

X_INLINE int XFontTexture::WidthCellCellCount(void) const
{ 
	return iWidthCellCount_; 
}

X_INLINE int XFontTexture::HeightCellCount(void) const
{ 
	return iHeightCellCount_; 
}

X_INLINE int XFontTexture::GetSlotUsage(void) 
{ 
	return wSlotUsage_; 
}

X_NAMESPACE_END
