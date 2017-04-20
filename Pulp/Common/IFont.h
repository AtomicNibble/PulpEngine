#pragma once

#ifndef _X_FONT_I_H_
#define _X_FONT_I_H_

#include <ITexture.h>
#include <IRender.h>

struct ICore;


X_NAMESPACE_DECLARE(engine, class IPrimativeContext);

X_NAMESPACE_BEGIN(font)



struct IFont;

struct IFontSys : public core::IEngineSysBase
{
	virtual ~IFontSys(){}
	
	virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_ABSTRACT;

	// Summary:
	//	 Creates a named font (case sensitive)
	virtual IFont* NewFont(const char* pFontName) X_ABSTRACT;
	// Summary:
	//	 Gets a named font (case sensitive)
	virtual IFont* GetFont(const char* pFontName) const X_ABSTRACT;

	// this should really take a sink no?
	virtual void ListFonts(void) const X_ABSTRACT;
};

#ifdef GetCharWidth
#undef GetCharWidth
#endif

X_DECLARE_FLAGS(DrawTextFlag)(
	CENTER,		// the center(hoz) of the text is placed at the draw pos, by default it's far left.
	CENTER_VER,	// the center(ver) of the text is placed at the draw pos, by default it's at the top.
	RIGHT,		// the end of the text (hoz) is placed at draw pos. (setting this will ignore 'CENTER', but 'CENTER_VER' is still valid)
	FRAMED,		// Draws a filled 65% opacity dark rectenagle under the text. aka a block background
	CLIP		// Clip the text against the provided rect, partial chars are drawn.

);

typedef Flags<DrawTextFlag> DrawTextFlags;

X_DECLARE_FLAG_OPERATORS(DrawTextFlags);

struct XTextDrawConect
{
	Color8u	col;  // 4
	Vec2f	size; // 8
	Rectf	clip; // 16
	float	widthScale; // 4
	int32_t effectId;

	DrawTextFlags flags;
	IFont* pFont;

	XTextDrawConect() :
		widthScale(1.f),
		size(16.f,16.f),
		effectId(-1),
		flags(0),
		pFont(nullptr)
	{}

	void Reset() { *this = XTextDrawConect(); }
	void SetSize(const Vec2f& _size) { size = _size; }
	void SetClip(const Rectf& rec) { clip = rec; }
	void SetColor(const Colorf& _col) { col = _col; }
	void SetColor(const Color8u& _col) { col = _col; }
	void SetCharWidthScale(const float scale) { widthScale = scale; }
	void SetEffectId(int32_t id) { effectId = id; }
	void SetDefaultEffect(void) { effectId = 0; }

	X_INLINE float GetCharWidth(void) const { return size.x; }
	X_INLINE float GetCharWidthScaled(void) const { return size.x * widthScale; }
	X_INLINE float GetCharHeight(void) const { return size.y; }
	X_INLINE float GetCharWidthScale(void) const { return widthScale; }
	X_INLINE int32_t GetEffectId(void) const { return effectId; }
};


typedef XTextDrawConect TextDrawContext;



X_DECLARE_ENUM(FontEncoding)(
	// Corresponds to the Unicode character set. This value covers all versions of the Unicode repertoire,
	// including ASCII and Latin-1. Most fonts include a Unicode charmap, but not all of them.
	Unicode,
	// Corresponds to the Microsoft Symbol encoding, used to encode mathematical symbols in the 32..255 character code range.
	// For more information, see `http://www.ceviz.net/symbol.htm'.
	MSSymbol
);


//
//	We support both ascii and wide char.
//	since it's not much work to support wide char
//  and saves me hassel later if I wanna support strange languages
//  most engines now a day support wide char like Blizzard
//  binary can still be multi-byte also.
struct IFont
{
	virtual ~IFont(){};
	virtual void Release(void) X_ABSTRACT;	// release the object
	virtual void Free(void) X_ABSTRACT;		// free internal memory
	virtual void FreeBuffers(void) X_ABSTRACT;		// free texture buffers
	virtual void FreeTexture(void) X_ABSTRACT;

	virtual bool loadFont(bool async) X_ABSTRACT;
	virtual void Reload(void) X_ABSTRACT;

	// blocks untill the font is ready to render.
	// returns false if loading / setup has failed.
	// only call this if you requested a async load, calling for sync loads is undefined.
	virtual bool WaitTillReady(void) X_ABSTRACT;


	// these draw the text into the primative context.
	virtual void DrawString(engine::IPrimativeContext* pPrimCon,
		const Vec3f& pos, const XTextDrawConect& contex, const char* pBegin, const char* pEnd) X_ABSTRACT;
	virtual void DrawString(engine::IPrimativeContext* pPrimCon,
		const Vec3f& pos, const XTextDrawConect& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_ABSTRACT;


	virtual size_t GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const X_ABSTRACT;
	virtual size_t GetTextLength(const wchar_t* pBegin, const wchar_t* pEnd, const bool asciiMultiLine) const X_ABSTRACT;

	// calculate the size.
	virtual Vec2f GetTextSize(const char* pBegin, const char* pEnd, const XTextDrawConect& contex) X_ABSTRACT;
	virtual Vec2f GetTextSize(const wchar_t* pBegin, const wchar_t* pEnd, const XTextDrawConect& contex) X_ABSTRACT;

	virtual int32_t GetEffectId(const char* pEffectName) const X_ABSTRACT;

//	virtual void GetGradientTextureCoord(float& minU, float& minV,
//		float& maxU, float& maxV) const X_ABSTRACT;
};



X_NAMESPACE_END



#endif // !_X_FONT_I_H_
