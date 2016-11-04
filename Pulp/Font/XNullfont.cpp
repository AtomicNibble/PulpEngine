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


void XFontNull::DrawString(const Vec2f& pos, const char* pStr, const XTextDrawConect& contex)
{
	X_UNUSED(pos);
	X_UNUSED(pStr);
	X_UNUSED(contex);
}

void XFontNull::DrawString(float x, float y, const char* pStr, const XTextDrawConect& contex)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(pStr);
	X_UNUSED(contex);
}

void XFontNull::DrawString(const Vec3f& pos, const char* pStr, const XTextDrawConect& contex)
{
	X_UNUSED(pos);
	X_UNUSED(pStr);
	X_UNUSED(contex);
}


void XFontNull::DrawStringW(const Vec2f& pos, const wchar_t* pStr, const XTextDrawConect& contex)
{
	X_UNUSED(pos);
	X_UNUSED(pStr);
	X_UNUSED(contex);
}

void XFontNull::DrawStringW(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& contex)
{
	X_UNUSED(pos);
	X_UNUSED(pStr);
	X_UNUSED(contex);
}



size_t XFontNull::GetTextLength(const char* pStr, const bool asciiMultiLine) const
{
	X_UNUSED(pStr);
	X_UNUSED(asciiMultiLine);
	return 0;
}

size_t XFontNull::GetTextLengthW(const wchar_t* pStr, const bool asciiMultiLine) const
{
	X_UNUSED(pStr);
	X_UNUSED(asciiMultiLine);
	return 0;
}


Vec2f XFontNull::GetTextSize(const char* pStr, const XTextDrawConect& contex)
{
	X_UNUSED(pStr);
	X_UNUSED(contex);
	return Vec2f::zero();
}

Vec2f XFontNull::GetTextSizeW(const wchar_t* pStr, const XTextDrawConect& contex)
{
	X_UNUSED(pStr);
	X_UNUSED(contex);

	return Vec2f::zero();
}


uint32_t XFontNull::GetEffectId(const char* pEffectName) const
{
	X_UNUSED(pEffectName);
	return 0;
}


void XFontNull::GetGradientTextureCoord(float& minU, float& minV,
	float& maxU, float& maxV) const
{
	X_UNUSED(minU);
	X_UNUSED(minV);
	X_UNUSED(maxU);
	X_UNUSED(maxV);
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