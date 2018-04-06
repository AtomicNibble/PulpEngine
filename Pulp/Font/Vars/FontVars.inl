

X_NAMESPACE_BEGIN(font)

X_INLINE int32_t FontVars::glyphCacheSize(void) const
{
    return glyphCacheSize_;
}

X_INLINE bool FontVars::glyphCachePreWarm(void) const
{
    return glyphCachePreWarm_ != 0;
}

X_INLINE bool FontVars::glyphCacheDebugRender(void) const
{
    return glyphDebugRender_ != 0;
}

X_INLINE bool FontVars::glyphDebugRect(void) const
{
    return glyphDebugRect_ != 0;
}

X_INLINE bool FontVars::debugRect(void) const
{
    return debugRect_ != 0;
}

X_INLINE bool FontVars::debugShowDrawPosition(void) const
{
    return debugShowDrawPosition_ != 0;
}

X_INLINE FontSmooth::Enum FontVars::fontSmoothMethod(void) const
{
    X_ASSERT(fontSmoothingMethod_ >= 0 && fontSmoothingMethod_ < FontSmooth::ENUM_COUNT, "Var out of range")(fontSmoothingMethod_);

    return static_cast<FontSmooth::Enum>(fontSmoothingMethod_);
}

X_INLINE FontSmoothAmount::Enum FontVars::fontSmoothAmount(void) const
{
    X_ASSERT(fontSmoothingAmount_ >= 0 && fontSmoothingAmount_ < FontSmoothAmount::ENUM_COUNT, "Var out of range")(fontSmoothingAmount_);

    return static_cast<FontSmoothAmount::Enum>(fontSmoothingAmount_);
}

X_NAMESPACE_END