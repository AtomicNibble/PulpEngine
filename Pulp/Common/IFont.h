#pragma once

#ifndef _X_FONT_I_H_
#define _X_FONT_I_H_

#include <ITexture.h>
#include <IRender.h>
#include <IConverterModule.h>

#include <Time\CompressedStamps.h>
#include <String\StringHash.h>

struct ICore;

X_NAMESPACE_DECLARE(engine, class IPrimativeContext);
X_NAMESPACE_DECLARE(core, struct FrameTimeData);

X_NAMESPACE_BEGIN(font)

static const char* FONT_DESC_FILE_EXTENSION = "font";
static const char* FONT_BAKED_FILE_EXTENSION = "fnt";

static const wchar_t FONT_PRECACHE_STR[] = L" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]"
                                           "^_`abcdefghijklmnopqrstuvwxyz{|}~¢£¤¥¦§¨©ª«¬­\n\t";

static const uint32_t FONT_MAX_LOADED = 8;


struct IFontLib : public IConverter
{
};

struct Metrics
{
    int32_t ascender;
    int32_t descender;
    int32_t max_advance;
};

struct IFont;

struct IFontSys : public core::IEngineSysBase
{
    virtual ~IFontSys() = default;

    // finish any async init tasks for all fonts.
    virtual bool asyncInitFinalize(void) X_ABSTRACT;

    virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_ABSTRACT;

    virtual IFont* loadFont(const char* pFontName) X_ABSTRACT;
    virtual IFont* findFont(const char* pFontName) const X_ABSTRACT;
    virtual IFont* getDefault(void) const X_ABSTRACT;

    virtual void releaseFont(IFont* pFont) X_ABSTRACT;

    virtual bool waitForLoad(IFont* pFont) X_ABSTRACT;

    // this should really take a sink no?
    virtual void listFonts(const char* pSearchPatten = nullptr) const X_ABSTRACT;
};

#ifdef GetCharWidth
#undef GetCharWidth
#endif

X_DECLARE_FLAGS(DrawTextFlag)
(
    CENTER,      // the center(hoz) of the text is placed at the draw pos, by default it's far left.
    CENTER_VER,  // the center(ver) of the text is placed at the draw pos, by default it's at the top.
    RIGHT,       // the end of the text (hoz) is placed at draw pos. (setting this will ignore 'CENTER', but 'CENTER_VER' is still valid)
    FRAMED,      // Draws a filled 65% opacity dark rectenagle under the text. aka a block background
    FRAMED_SNUG, // includes descenders and moves top to max ascender.
    CLIP         // Clip the text against the provided rect, partial chars are drawn.

);

typedef Flags<DrawTextFlag> DrawTextFlags;

X_DECLARE_FLAG_OPERATORS(DrawTextFlags);

struct TextDrawContext
{
    Color8u col;      // 4
    Vec2f size;       // 8
    Rectf clip;       // 16
    float widthScale; // 4
    int32_t effectId;

    DrawTextFlags flags;
    IFont* pFont;

    X_INLINE TextDrawContext() :
        size(16.f, 16.f),
        widthScale(1.f),
        effectId(0),
        flags(0),
        pFont(nullptr)
    {
    }

    X_INLINE void SetSize(const Vec2f& _size)
    {
        size = _size;
    }
    X_INLINE void SetClip(const Rectf& rec)
    {
        clip = rec;
    }
    X_INLINE void SetColor(const Colorf& _col)
    {
        col = _col;
    }
    X_INLINE void SetColor(const Color8u& _col)
    {
        col = _col;
    }
    X_INLINE void SetCharWidthScale(const float scale)
    {
        widthScale = scale;
    }
    X_INLINE void SetEffectId(int32_t id)
    {
        effectId = id;
    }
    X_INLINE void SetDefaultEffect(void)
    {
        effectId = 0;
    }

    X_INLINE float GetCharHeight(void) const
    {
        return size.y;
    }
    X_INLINE int32_t GetEffectId(void) const
    {
        return effectId;
    }
};

X_DECLARE_ENUM(FontEncoding)
(
    // Corresponds to the Unicode character set. This value covers all versions of the Unicode repertoire,
    // including ASCII and Latin-1. Most fonts include a Unicode charmap, but not all of them.
    Unicode,
    // Corresponds to the Microsoft Symbol encoding, used to encode mathematical symbols in the 32..255 character code range.
    // For more information, see `http://www.ceviz.net/symbol.htm'.
    MSSymbol);

X_DECLARE_FLAGS8(FontFlag)
(
    // The font is proportional, aka not monospace.
    PROPORTIONAL,
    SDF
);

typedef Flags8<FontFlag> FontFlags;

//
//	We support both ascii and wide char.
//	since it's not much work to support wide char
//  and saves me hassel later if I wanna support strange languages
//  most engines now a day support wide char like Blizzard
//  binary can still be multi-byte also.
struct IFont
{
    virtual ~IFont() = default;
    virtual void Free(void) X_ABSTRACT;        // free internal memory
    virtual void FreeTexture(void) X_ABSTRACT;

    // draw a load of test text.
    virtual void DrawTestText(engine::IPrimativeContext* pPrimCon, const core::FrameTimeData& time) X_ABSTRACT;

    // these draw the text into the primative context.
    virtual void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_ABSTRACT;
    virtual void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_ABSTRACT;
    virtual void DrawString(engine::IPrimativeContext* pPrimCon,
        const Vec3f& pos, const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_ABSTRACT;
    virtual void DrawString(engine::IPrimativeContext* pPrimCon,
        const Vec3f& pos, const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_ABSTRACT;

    virtual size_t GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const X_ABSTRACT;
    virtual size_t GetTextLength(const wchar_t* pBegin, const wchar_t* pEnd, const bool asciiMultiLine) const X_ABSTRACT;

    // calculate the size.
    virtual Vec2f GetTextSize(const char* pBegin, const char* pEnd, const TextDrawContext& contex) X_ABSTRACT;
    virtual Vec2f GetTextSize(const wchar_t* pBegin, const wchar_t* pEnd, const TextDrawContext& contex) X_ABSTRACT;

    // size of N chars, for none monospace fonts it just uses space.
    virtual float32_t GetCharWidth(wchar_t cChar, size_t num, const TextDrawContext& contex) X_ABSTRACT;

    virtual int32_t GetEffectId(const char* pEffectName) const X_ABSTRACT;

    //	virtual void GetGradientTextureCoord(float& minU, float& minV,
    //		float& maxU, float& maxV) const X_ABSTRACT;
};

// File types
struct GlyphHdr
{
    wchar_t currentChar;

    uint16_t advanceX;
    uint8_t charWidth;   // size in pixel
    uint8_t charHeight;  // size in pixel
                         // these are 16bit as can be big if downsampling.
    int16_t charOffsetX; // these can be negative.
    int16_t charOffsetY;

    int8_t bitmapOffsetX;
    int8_t bitmapOffsetY;
    uint8_t _pad[4];
};

struct FontHdr
{
    static const uint32_t X_FONT_BIN_FOURCC = X_TAG('X', 'B', 'K', 'F');
    static const uint32_t X_FONT_BIN_VERSION = 1;

    size_t glyphDataSize(void) const {
        return numGlyphs * ((glyphHeight * glyphWidth) + sizeof(GlyphHdr));
    }

    X_INLINE bool isValid(void) const {
        if (version != X_FONT_BIN_VERSION) {
            X_ERROR("Font", "model version is invalid. FileVer: %" PRIu8 " RequiredVer: %" PRIu32,
                version, X_FONT_BIN_VERSION);
        }

        return forcc == X_FONT_BIN_FOURCC && version == X_FONT_BIN_VERSION;
    }

    uint32_t forcc;
    uint8_t version;
    FontFlags flags;
    uint8_t numEffects;
    uint8_t numPasses;
    core::DateTimeStampSmall modifed;

    uint16_t numGlyphs;
    uint16_t glyphWidth;
    uint16_t glyphHeight;

    Metrics metrics;

    uint32_t sourceFontSize;
};

struct FontEffectHdr
{
    int8_t numPass;
    int8_t passStartIdx;
    int8_t _pad[2];
    core::StrHash nameHash;
};

struct FontPass
{
    Color8u col;
    Vec2f offset;
};

X_ENSURE_SIZE(FontHdr, 36);
X_ENSURE_SIZE(GlyphHdr, 16);
X_ENSURE_SIZE(FontEffectHdr, 8);
X_ENSURE_SIZE(FontPass, 12);

X_NAMESPACE_END

#endif // !_X_FONT_I_H_
