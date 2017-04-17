
X_NAMESPACE_BEGIN(font)

X_INLINE const Vec2i XFontTexture::GetSize(void) const 
{ 
	return Vec2i(width_, height_);
}

X_INLINE const int32_t XFontTexture::GetWidth(void) const
{ 
	return width_;
}

X_INLINE const int32_t XFontTexture::GetHeight(void) const
{ 
	return height_; 
}

X_INLINE const XFontTexture::BufferArr& XFontTexture::GetBuffer(void) const
{
	return textureBuffer_;
}

X_INLINE float XFontTexture::GetTextureCellWidth(void) const
{ 
	return textureCellWidth_;
}

X_INLINE float XFontTexture::GetTextureCellHeight(void) const 
{ 
	return textureCellHeight_;
}

X_INLINE int32_t XFontTexture::WidthCellCellCount(void) const
{ 
	return widthCellCount_; 
}

X_INLINE int32_t XFontTexture::HeightCellCount(void) const
{ 
	return heightCellCount_; 
}

X_INLINE int32_t XFontTexture::GetSlotUsage(void)
{ 
	return slotUsage_; 
}

X_NAMESPACE_END
