#include "stdafx.h"
#include "XNullfont.h"

X_NAMESPACE_BEGIN(font)

#if defined(X_USE_NULLFONT)

XFontNull XFontSysNull::nullFont_;

// -------------------------------------------------------------

void XFontNull::Release(void)
{
}
// release the object
void XFontNull::Free(void)
{
}
// free internal memory
void XFontNull::FreeBuffers(void)
{
}
// free texture buffers
void XFontNull::FreeTexture(void)
{
}

bool XFontNull::loadFont(void)
{
    return true;
}

texture::TexID XFontNull::getTextureId(void) const
{
    return -1;
}

void XFontNull::DrawString(engine::IPrimativeContext* pPrimCon, render::StateHandle stateHandle, const Vec3f& pos,
    const XTextDrawConect& contex, const char* pBegin, const char* pEnd)
{
    X_UNUSED(pPrimCon);
    X_UNUSED(stateHandle);
    X_UNUSED(pos);
    X_UNUSED(contex);
    X_UNUSED(pBegin);
    X_UNUSED(pEnd);
}

void XFontNull::DrawString(engine::IPrimativeContext* pPrimCon, render::StateHandle stateHandle, const Vec3f& pos,
    const XTextDrawConect& contex, const wchar_t* pBegin, const wchar_t* pEnd)
{
    X_UNUSED(pPrimCon);
    X_UNUSED(stateHandle);
    X_UNUSED(pos);
    X_UNUSED(contex);
    X_UNUSED(pBegin);
    X_UNUSED(pEnd);
}

size_t XFontNull::GetTextLength(const char* pStr, const bool asciiMultiLine) const
{
    X_UNUSED(pStr);
    X_UNUSED(asciiMultiLine);
    return 0;
}

size_t XFontNull::GetTextLength(const wchar_t* pStr, const bool asciiMultiLine) const
{
    X_UNUSED(pStr);
    X_UNUSED(asciiMultiLine);
    return 0;
}

// calculate the size.
Vec2f XFontNull::GetTextSize(const char* pStr, const XTextDrawConect& contex)
{
    X_UNUSED(pStr);
    X_UNUSED(contex);
    return Vec2f::zero();
}

Vec2f XFontNull::GetTextSize(const wchar_t* pStr, const XTextDrawConect& contex)
{
    X_UNUSED(pStr);
    X_UNUSED(contex);
    return Vec2f::zero();
}

int32_t XFontNull::GetEffectId(const char* pEffectName) const
{
    X_UNUSED(pEffectName);
    return 0;
}

// -------------------------------------------------------------

bool XFontSysNull::Init(void)
{
    return true;
}

void XFontSysNull::ShutDown(void)
{
}

void XFontSysNull::release(void)
{
    X_DELETE(this, g_fontArena);
}

void XFontSysNull::updateDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const
{
    X_UNUSED(bucket);
}

IFont* XFontSysNull::NewFont(const char* pFontName)
{
    X_UNUSED(pFontName);
    return &nullFont_;
}

IFont* XFontSysNull::GetFont(const char* pFontName) const
{
    X_UNUSED(pFontName);
    return &nullFont_;
}

void XFontSysNull::ListFontNames(void) const
{
}

#else

X_DISABLE_EMPTY_FILE_WARNING

#endif // X_USE_NULLFONT

X_NAMESPACE_END