
X_NAMESPACE_BEGIN(engine)

X_INLINE bool TextureVars::allowRawImgLoading(void) const
{
    return allowRawImgLoading_ != 0;
}

X_INLINE bool TextureVars::allowFmtDDS(void) const
{
    return allowFmtDDS_ != 0;
}
X_INLINE bool TextureVars::allowFmtPNG(void) const
{
    return allowFmtPNG_ != 0;
}
X_INLINE bool TextureVars::allowFmtJPG(void) const
{
    return allowFmtJPG_ != 0;
}
X_INLINE bool TextureVars::allowFmtPSD(void) const
{
    return allowFmtPSD_ != 0;
}
X_INLINE bool TextureVars::allowFmtTGA(void) const
{
    return allowFmtTGA_ != 0;
}

X_NAMESPACE_END