#pragma once

#ifndef _X_FONT_NULL_H_
#define _X_FONT_NULL_H_

X_NAMESPACE_BEGIN(font)

#if defined(X_USE_NULLFONT)

class XFontNull : public IFont
{
public:
    ~XFontNull() X_FINAL = default;

    virtual void Free(void) X_FINAL;        // free internal memory
    virtual void FreeTexture(void) X_FINAL;

    // draw a load of test text.
    virtual void DrawTestText(engine::IPrimitiveContext* pPrimCon, const core::FrameTimeData& time) X_FINAL;

    // these draw the text into the primitive context.
    virtual void DrawString(engine::IPrimitiveContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_FINAL;
    virtual void DrawString(engine::IPrimitiveContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
        const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;
    virtual void DrawString(engine::IPrimitiveContext* pPrimCon,
        const Vec3f& pos, const TextDrawContext& contex, const char* pBegin, const char* pEnd) X_FINAL;
    virtual void DrawString(engine::IPrimitiveContext* pPrimCon,
        const Vec3f& pos, const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_FINAL;

    virtual size_t GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const X_FINAL;

    // calculate the size.
    virtual Vec2f GetTextSize(const char* pBegin, const char* pEnd, const TextDrawContext& contex) X_FINAL;

    // size of N chars, for none monospace fonts it just uses space.
    virtual float32_t GetCharWidth(char cChar, size_t num, const TextDrawContext& contex) X_FINAL;

    virtual int32_t GetEffectId(const char* pEffectName) const X_FINAL;
};

class XFontSysNull : public IFontSys
{
public:
    ~XFontSysNull() X_FINAL = default;

    virtual void registerVars(void) X_FINAL;
    virtual void registerCmds(void) X_FINAL;

    virtual bool init(void) X_FINAL;
    virtual void shutDown(void) X_FINAL;
    virtual void release(void) X_FINAL;

    virtual bool asyncInitFinalize(void) X_FINAL;

    virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_FINAL;

    virtual IFont* loadFont(core::string_view name) X_FINAL;
    virtual IFont* findFont(core::string_view name) const X_FINAL;
    virtual IFont* getDefault(void) const X_FINAL;

    virtual void releaseFont(IFont* pFont) X_FINAL;

    virtual bool waitForLoad(IFont* pFont) X_FINAL;

    // this should really take a sink no?
    virtual void listFonts(core::string_view searchPattern) const X_FINAL;

private:
    static XFontNull nullFont_;
};

#endif // X_USE_NULLFONT

X_NAMESPACE_END

#endif // !_X_FONT_NULL_H_
