#pragma once

#ifndef _X_FONT_SYS_H_
#define _X_FONT_SYS_H_

#include "FontTypes.h"

#include <Containers\FixedArray.h>
#include <Containers\Array.h>
#include <Threading\Signal.h>

#include <Util\UniquePointer.h>
#include <IFileSys.h>

#include <Assets\AssetBase.h>

struct Vertex_P3F_T2F_C4B;

X_NAMESPACE_DECLARE(engine,
                    class Material);
X_NAMESPACE_DECLARE(core,
                    struct IoRequestBase;
                    struct XFileAsync);

X_NAMESPACE_BEGIN(font)

class XFontTexture;
class XFontSystem;

class XFont : public IFont, public core::AssetBase
{
public:
public:
    XFont(core::string_view name, XFontSystem& fontSys);
    ~XFont();

    // IFont
    void Free(void) X_FINAL;
    void FreeTexture(void) X_FINAL;

    bool isReady(void);

    bool processData(core::UniquePointer<char[]> data, uint32_t dataSize);

    void DrawTestText(engine::IPrimitiveContext* pPrimCon, const core::FrameTimeData& time) X_FINAL;

    void DrawString(engine::IPrimitiveContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_FINAL;
    void DrawString(engine::IPrimitiveContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;
    void DrawString(engine::IPrimitiveContext* pPrimCon, const Vec3f& pos,
        const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_FINAL;
    void DrawString(engine::IPrimitiveContext* pPrimCon, const Vec3f& pos,
        const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;

    size_t GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const X_FINAL;

    // calculate the size.
    Vec2f GetTextSize(const char* pBegin, const char* pEnd, const TextDrawContext& contex) X_FINAL;

    // size of N chars, for none monospace fonts it just uses space.
    float32_t GetCharWidth(char cChar, size_t num, const TextDrawContext& contex) X_FINAL;

    int32_t GetEffectId(const char* pEffectName) const X_FINAL;

    // ~IFont

    void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const;

    X_INLINE FontFlags getFlags(void) const;
    X_INLINE bool isDirty(void) const;
    X_INLINE XFontTexture* getFontTexture(void) const;

    void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket);

private:
    void DrawStringInternal(engine::IPrimitiveContext* pPrimCon, const Vec3f& pos,
        const TextDrawContext& contex, const char* pBegin, const char* pEnd, const Matrix33f* pRotation);

private:
    Vec2f GetTextSizeInternal(const char* pBegin, const char* pEnd, const TextDrawContext& contex);

    bool CreateDeviceTexture(void);
    void Prepare(const char* pBegin, const char* pEnd);

private:
    static size_t ByteToWide(const char* pBegin, const char* pEnd, wchar_t* pOut, const size_t buflen);

private:
    XFontSystem& fontSys_;

    core::UniquePointer<char[]> data_;
    core::span<const FontEffectHdr> effectsHdr_;
    core::span<const FontPass> effectsPasses_;
    core::span<const GlyphHdr> bakedGlyphs_;
    core::span<const char> bakedData_;
    core::span<const uint8_t> fontSrc_;

    FontFlags flags_;

    // the cpu texture
    core::UniquePointer<XFontTexture> pFontTexture_;

    render::IDeviceTexture* pTexture_;
    bool fontTexDirty_;

    // shader and state.
    engine::Material* pMaterial_;
};

X_INLINE FontFlags XFont::getFlags(void) const
{
    return flags_;
}

X_INLINE bool XFont::isDirty(void) const
{
    return fontTexDirty_;
}

X_INLINE XFontTexture* XFont::getFontTexture(void) const
{
    return pFontTexture_.ptr();
}

X_NAMESPACE_END

#endif // !_X_FONT_SYS_H_
