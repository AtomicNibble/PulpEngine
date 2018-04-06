#pragma once

#include <IFont.h>

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(font)

class FontVars
{
public:
    FontVars();
    ~FontVars() = default;

    void registerVars(void);

    X_INLINE int32_t glyphCacheSize(void) const;
    X_INLINE bool glyphCachePreWarm(void) const;
    X_INLINE bool glyphCacheDebugRender(void) const;
    X_INLINE bool glyphDebugRect(void) const;
    X_INLINE bool debugRect(void) const;
    X_INLINE bool debugShowDrawPosition(void) const;

    X_INLINE FontSmooth::Enum fontSmoothMethod(void) const;
    X_INLINE FontSmoothAmount::Enum fontSmoothAmount(void) const;

private:
    int32_t glyphCacheSize_;
    int32_t glyphCachePreWarm_;
    int32_t glyphDebugRender_;
    int32_t glyphDebugRect_;
    int32_t debugRect_;
    int32_t debugShowDrawPosition_;

    int32_t fontSmoothingMethod_;
    int32_t fontSmoothingAmount_;
};

X_NAMESPACE_END

#include "FontVars.inl"