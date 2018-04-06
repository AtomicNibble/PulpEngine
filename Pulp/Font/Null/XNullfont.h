#pragma once

#ifndef _X_FONT_NULL_H_
#define _X_FONT_NULL_H_

X_NAMESPACE_BEGIN(font)

#if defined(X_USE_NULLFONT)

class XFontNull : public IFont
{
public:
    ~XFontNull() X_OVERRIDE = default;

    void Release(void) X_OVERRIDE;     // release the object
    void Free(void) X_OVERRIDE;        // free internal memory
    void FreeBuffers(void) X_OVERRIDE; // free texture buffers
    void FreeTexture(void) X_OVERRIDE;

    texture::TexID getTextureId(void) const X_OVERRIDE;

    bool loadFont(void) X_OVERRIDE;

    void DrawString(engine::IPrimativeContext* pPrimCon, render::StateHandle stateHandle, const Vec3f& pos,
        const XTextDrawConect& contex, const char* pBegin, const char* pEnd) X_OVERRIDE;
    void DrawString(engine::IPrimativeContext* pPrimCon, render::StateHandle stateHandle, const Vec3f& pos,
        const XTextDrawConect& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_OVERRIDE;

    size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const X_OVERRIDE;
    size_t GetTextLength(const wchar_t* pStr, const bool asciiMultiLine) const X_OVERRIDE;

    // calculate the size.
    Vec2f GetTextSize(const char* pStr, const XTextDrawConect& contex) X_OVERRIDE;
    Vec2f GetTextSize(const wchar_t* pStr, const XTextDrawConect& contex) X_OVERRIDE;

    int32_t GetEffectId(const char* pEffectName) const X_OVERRIDE;
};

class XFontSysNull : public IFontSys
{
public:
    ~XFontSysNull() X_OVERRIDE = default;

    virtual bool Init(void) X_OVERRIDE;
    virtual void ShutDown(void) X_OVERRIDE;
    virtual void release(void) X_OVERRIDE;

    virtual void updateDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_OVERRIDE;

    virtual IFont* NewFont(const char* pFontName) X_OVERRIDE;
    virtual IFont* GetFont(const char* pFontName) const X_OVERRIDE;

    virtual void ListFontNames(void) const X_OVERRIDE;

private:
    static XFontNull nullFont_;
};

#endif // X_USE_NULLFONT

X_NAMESPACE_END

#endif // !_X_FONT_NULL_H_
