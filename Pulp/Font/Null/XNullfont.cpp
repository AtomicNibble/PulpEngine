#include "stdafx.h"
#include "XNullfont.h"

X_NAMESPACE_BEGIN(font)

#if defined(X_USE_NULLFONT)

XFontNull XFontSysNull::nullFont_;

// -------------------------------------------------------------

void XFontNull::Free(void)
{
}

void XFontNull::FreeTexture(void)
{
}

void XFontNull::DrawTestText(engine::IPrimativeContext* pPrimCon, const core::FrameTimeData& time)
{
    X_UNUSED(pPrimCon, time);
}

// these draw the text into the primative context.
void XFontNull::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
    const TextDrawContext& contex, const char* pBegin, const char* pEnd)
{
    X_UNUSED(pPrimCon, pos, ang, contex, pBegin, pEnd);
}

void XFontNull::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
    const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd)
{
    X_UNUSED(pPrimCon, pos, ang, contex, pBegin, pEnd);
}

void XFontNull::DrawString(engine::IPrimativeContext* pPrimCon,
    const Vec3f& pos, const TextDrawContext& contex, const char* pBegin, const char* pEnd)
{
    X_UNUSED(pPrimCon, pos, contex, pBegin, pEnd);

}

void XFontNull::DrawString(engine::IPrimativeContext* pPrimCon,
    const Vec3f& pos, const TextDrawContext& contex, const wchar_t* pBegin, const wchar_t* pEnd)
{
    X_UNUSED(pPrimCon, pos, contex, pBegin, pEnd);

}

size_t XFontNull::GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const
{
    X_UNUSED(pBegin, pEnd, asciiMultiLine);

    return 0;
}

// calculate the size.
Vec2f XFontNull::GetTextSize(const char* pBegin, const char* pEnd, const TextDrawContext& contex)
{
    X_UNUSED(pBegin, pEnd, contex);
    return Vec2f::zero();
}

// size of N chars, for none monospace fonts it just uses space.
float32_t XFontNull::GetCharWidth(char cChar, size_t num, const TextDrawContext& contex)
{
    X_UNUSED(cChar, num, contex);
    return 0.f;
}



int32_t XFontNull::GetEffectId(const char* pEffectName) const
{
    X_UNUSED(pEffectName);
    return 0;
}

// -------------------------------------------------------------

void XFontSysNull::registerVars(void)
{

}

void XFontSysNull::registerCmds(void)
{

}

bool XFontSysNull::init(void)
{
    return true;
}

void XFontSysNull::shutDown(void)
{

}

void XFontSysNull::release(void)
{

}

bool XFontSysNull::asyncInitFinalize(void)
{
    return true;
}

void XFontSysNull::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const
{
    X_UNUSED(bucket);
}

IFont* XFontSysNull::loadFont(core::string_view name)
{
    X_UNUSED(name);
    return nullptr;
}

IFont* XFontSysNull::findFont(core::string_view name) const
{
    X_UNUSED(name);
    return nullptr;
}

IFont* XFontSysNull::getDefault(void) const
{
    return nullptr;
}

void XFontSysNull::releaseFont(IFont* pFont)
{
    X_UNUSED(pFont);
}

bool XFontSysNull::waitForLoad(IFont* pFont)
{
    X_UNUSED(pFont);
    return true;
}

// this should really take a sink no?
void XFontSysNull::listFonts(core::string_view searchPattern) const
{
    X_UNUSED(searchPattern);
}

#else

X_DISABLE_EMPTY_FILE_WARNING

#endif // X_USE_NULLFONT

X_NAMESPACE_END
