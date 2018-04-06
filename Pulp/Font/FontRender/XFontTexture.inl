
X_NAMESPACE_BEGIN(font)

X_INLINE bool XFontTexture::IsReady(void) const
{
    // once the cache is ready we are ready.
    return loadStatus_ == core::LoadStatus::Complete && glyphCache_.IsLoaded();
}

X_INLINE const SourceNameStr& XFontTexture::GetName(void) const
{
    return name_;
}

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

X_INLINE int32_t XFontTexture::GetCellWidth(void) const
{
    return cellWidth_;
}

X_INLINE int32_t XFontTexture::GetCellHeight(void) const
{
    return cellHeight_;
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

X_INLINE int32_t XFontTexture::GetCacheMisses(void)
{
    return cacheMisses_;
}

X_INLINE const Metrics& XFontTexture::GetMetrics(void) const
{
    return glyphCache_.GetMetrics();
}

X_NAMESPACE_END
