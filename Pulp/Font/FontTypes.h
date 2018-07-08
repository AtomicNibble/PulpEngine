#pragma once

#include <String\StackString.h>

X_NAMESPACE_BEGIN(font)


struct FontEffect
{
    core::StackString<64> name;
    core::FixedArray<FontPass, MAX_FONT_PASS> passes;
};

typedef core::StackString<assetDb::ASSET_NAME_MAX_LENGTH> FontNameStr;
typedef core::StackString<128> SourceNameStr;
typedef core::Array<FontEffect> EffetsArr;

X_DECLARE_ENUM(FontSmooth)
(
    NONE,
    BLUR,       // Smooth by blurring it.
    SUPERSAMPLE // Smooth by rendering the characters into a bigger texture, and then resize it to the normal size using bilinear filtering.
);

X_DECLARE_ENUM(FontSmoothAmount)
(
    NONE,
    X2,
    X4);

X_NAMESPACE_END