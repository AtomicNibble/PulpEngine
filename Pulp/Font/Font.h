#pragma once

#ifndef _X_FONT_SYS_H_
#define _X_FONT_SYS_H_

#include "FontTypes.h"

#include <Containers\FixedArray.h>
#include <Containers\Array.h>
#include <Threading\Signal.h>

#include <Util\UniquePointer.h>
#include <IFileSys.h>

struct Vertex_P3F_T2F_C4B;

X_NAMESPACE_DECLARE(engine,
                    class Material;);
X_NAMESPACE_DECLARE(core,
                    struct IoRequestBase;
                    struct XFileAsync;);

X_NAMESPACE_BEGIN(font)

class XFontTexture;
class XFontSystem;

class XFont : public IFont
{
public:
public:
    XFont(XFontSystem& fontSys, const char* pFontName);
    ~XFont();

    // IFont
    void Release(void) X_FINAL;
    void Free(void) X_FINAL;
    void FreeBuffers(void) X_FINAL;
    void FreeTexture(void) X_FINAL;

    bool loadFont(bool async) X_FINAL;
    void Reload(void) X_FINAL;

    bool isReady(void);
    bool WaitTillReady(void) X_FINAL;

    void DrawTestText(engine::IPrimativeContext* pPrimCon, const core::FrameTimeData& time) X_FINAL;

    void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_FINAL;
    void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;
    void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
        const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_FINAL;
    void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
        const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;

    size_t GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const X_FINAL;
    size_t GetTextLength(const wchar_t* pBegin, const wchar_t* pEnd, const bool asciiMultiLine) const X_FINAL;

    // calculate the size.
    Vec2f GetTextSize(const char* pBegin, const char* pEnd, const TextDrawContext& contex) X_FINAL;
    Vec2f GetTextSize(const wchar_t* pBegin, const wchar_t* pEnd, const TextDrawContext& contex) X_FINAL;

    // size of N chars, for none monospace fonts it just uses space.
    float32_t GetCharWidth(wchar_t cChar, size_t num, const TextDrawContext& contex) X_FINAL;

    int32_t GetEffectId(const char* pEffectName) const X_FINAL;

    // ~IFont

    void GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const;

    X_INLINE const FontNameStr& getName(void) const;
    X_INLINE FontFlags getFlags(void) const;
    X_INLINE bool isDirty(void) const;
    X_INLINE XFontTexture* getFontTexture(void) const;

    void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket);

private:
    void DrawStringInternal(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
        const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd, const Matrix33f* pRotation);

private:
    // loading logic
    void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);

    void ProcessFontFile_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

    static bool processXmlData(const char* pBegin, const char* pEnd, SourceNameStr& sourceNameOut,
        EffetsArr& effectsOut, FontFlags& flags);

    bool loadFontDef(bool async);

private:
    Vec2f GetTextSizeWInternal(const wchar_t* pBegin, const wchar_t* pEnd, const TextDrawContext& contex);

    bool CreateDeviceTexture(void);
    void Prepare(const wchar_t* pBegin, const wchar_t* pEnd);

private:
    static size_t ByteToWide(const char* pBegin, const char* pEnd, wchar_t* pOut, const size_t buflen);

private:
    XFontSystem& fontSys_;
    FontNameStr name_;
    SourceNameStr sourceName_;
    FontFlags flags_;

    EffetsArr effects_;

    // the cpu texture
    XFontTexture* pFontTexture_;

    render::IDeviceTexture* pTexture_;
    bool fontTexDirty_;

    // shader and state.
    engine::Material* pMaterial_;

    // loading
    core::Signal signal_;
    core::LoadStatus::Enum loadStatus_;
};

X_INLINE const FontNameStr& XFont::getName(void) const
{
    return name_;
}

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
    return pFontTexture_;
}

X_NAMESPACE_END

#endif // !_X_FONT_SYS_H_
