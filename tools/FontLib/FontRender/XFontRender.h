#pragma once

#ifndef _X_FONT_RENDER_H_
#define _X_FONT_RENDER_H_

#include <Util\UniquePointer.h>

#define FT_EXPORT(x) x
// #define FT_EXPORT_DEF(x) x
// #define FT_EXPORT_VAR( x ) x
#include <ft2build.h>
#include FT_FREETYPE_H

X_NAMESPACE_BEGIN(font)

class XGlyphBitmap;
struct XGlyph;

// uses free type to render glyphs for a font.
class XFontRender
{
    X_NO_COPY(XFontRender);
    X_NO_ASSIGN(XFontRender);

    typedef core::Array<int16_t> Int16Arr;
    typedef core::Array<double> DoubleArr;

public:
    FONTLIB_EXPORT XFontRender(core::MemoryArenaBase* arena);
    FONTLIB_EXPORT ~XFontRender();

    FONTLIB_EXPORT bool SetRawFontBuffer(core::UniquePointer<uint8_t[]> data, int32_t length, FontEncoding::Enum encoding);
    FONTLIB_EXPORT bool Release(void);

    FONTLIB_EXPORT bool GetGlyph(XGlyph& glphy, XGlyphBitmap& destBitMap, wchar_t charCode, bool sdf);

    X_INLINE bool ValidFace(void) const;
    X_INLINE void EnabledDebugRender(bool enable);

    // scale the glyphs.
    X_INLINE void SetSizeRatio(float sizeRatio);
    X_INLINE float GetSizeRatio(void) const;

    FONTLIB_EXPORT bool SetEncoding(FontEncoding::Enum encoding);
    X_INLINE FontEncoding::Enum GetEncoding(void) const;

    FONTLIB_EXPORT void SetGlyphBitmapSize(int32_t width, int32_t height, float sizeRatio);
    FONTLIB_EXPORT void GetGlyphBitmapSize(int32_t* pWidth, int32_t* pHeight) const;

    X_INLINE const Metrics& GetMetrics(void) const;

private:
    void GenerateSDF(XGlyphBitmap& bitMap);
    void makeDistanceMapd(DoubleArr& data, uint32_t width, uint32_t height);

private:
    static const char* errToStr(FT_Error err);

private:
    core::MemoryArenaBase* arena_;
    core::UniquePointer<uint8_t[]> data_; // must stay valid for lifetime of FT_Face

    FT_Library pLibrary_;
    FT_Face pFace_;
    FontEncoding::Enum encoding_;

    bool debugRender_;
    float sizeRatio_;

    int32_t glyphBitmapWidth_;
    int32_t glyphBitmapHeight_;

    Metrics metrics_;

    // sdf stuff.
    Int16Arr xdist_;
    Int16Arr ydist_;
    DoubleArr gx_;
    DoubleArr gy_;
    DoubleArr outside_;
    DoubleArr inside_;
    DoubleArr tmpData_;
};

X_NAMESPACE_END

#include "XFontRender.inl"

#endif // !_X_FONT_RENDER_H_
