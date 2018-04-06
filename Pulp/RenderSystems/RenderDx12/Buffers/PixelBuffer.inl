
X_NAMESPACE_BEGIN(render)

X_INLINE::texture::Texture& PixelBuffer::getTex(void)
{
    return textInst_;
}

X_INLINE const ::texture::Texture& PixelBuffer::getTex(void) const
{
    return textInst_;
}

X_NAMESPACE_END