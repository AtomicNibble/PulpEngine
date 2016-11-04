#pragma once

#ifndef _X_FONT_I_H_
#define _X_FONT_I_H_

struct ICore;



X_NAMESPACE_BEGIN(font)

#define TTFFLAG_SMOOTH_NONE			0x00000000		// No smooth.
#define TTFFLAG_SMOOTH_BLUR			0x00000001		// Smooth by blurring it.
#define TTFFLAG_SMOOTH_SUPERSAMPLE	0x00000002		// Smooth by rendering the characters into a bigger texture, and then resize it to the normal size using bilinear filtering.

#define TTFFLAG_SMOOTH_MASK			0x0000000f		// Mask for retrieving.
#define TTFFLAG_SMOOTH_SHIFT		0				// Shift amount for retrieving.

#define TTFLAG_SMOOTH_AMOUNT_2X		0x00010000		// Blur / supersample [2x]
#define TTFLAG_SMOOTH_AMOUNT_4X		0x00020000		// Blur / supersample [4x]

#define TTFFLAG_SMOOTH_AMOUNT_MASK	0x000f0000		// Mask for retrieving.
#define TTFFLAG_SMOOTH_AMOUNT_SHIFT	16				// Shift amount for retrieving.


struct IFont;

struct IFontSys
{
	virtual ~IFontSys(){}
	
	virtual bool Init(void) X_ABSTRACT;
	virtual void ShutDown(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;


	// Summary:
	//	 Creates a named font (case sensitive)
	virtual IFont* NewFont(const char* pFontName) X_ABSTRACT;
	// Summary:
	//	 Gets a named font (case sensitive)
	virtual IFont* GetFont(const char* pFontName) const X_ABSTRACT;

	// this should really take a sink no?
	virtual void ListFontNames(void) const X_ABSTRACT;
};

#ifdef GetCharWidth
#undef GetCharWidth
#endif

X_DECLARE_FLAGS(DrawTextFlag)(
	CENTER,
	RIGHT,
	CENTER_VER,
	MONOSPACE,
	POS_2D,
	FIXED_SIZE,
	FRAMED,
	CLIP,
	SCALE_800x600
);

typedef Flags<DrawTextFlag> DrawTextFlags;

struct XTextDrawConect
{
	Color8u	col;  // 4
	Vec2f	size; // 8
	Rectf	clip; // 16
	float	widthScale; // 4
	uint32_t effectId;

	DrawTextFlags flags;
	IFont* pFont;

	XTextDrawConect() :
		widthScale(1.f),
		size(16.f,16.f),
		effectId(0),
		flags(0),
		pFont(nullptr)
	{}

	void Reset() { *this = XTextDrawConect(); }
	void SetSize(const Vec2f& _size) { size = _size; }
	void SetClip(const Rectf& rec) { clip = rec; }
	void SetColor(const Colorf& _col) { col = _col; }
	void SetColor(const Color8u& _col) { col = _col; }
	void SetCharWidthScale(const float scale) { widthScale = scale; }
	void SetEffectId(uint32_t id) { effectId = id; }
	void SetDefaultEffect(void) { effectId = 0; }

	X_INLINE float GetCharWidth(void) const { return size.x; }
	X_INLINE float GetCharWidthScaled(void) const { return size.x * widthScale; }
	X_INLINE float GetCharHeight(void) const { return size.y; }
	X_INLINE float GetCharWidthScale(void) const { return widthScale; }
	X_INLINE uint32_t GetEffectId(void) const { return effectId; }
};


typedef XTextDrawConect TextDrawContext;

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

	virtual texture::TexID getTextureId(void) const X_ABSTRACT;


	virtual bool loadFont(void) X_ABSTRACT;

	// these draw the text into the primative context.
	virtual void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const XTextDrawConect& contex, const char* pBegin, const char* pEnd) X_ABSTRACT;
	virtual void DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const XTextDrawConect& contex, const wchar_t* pBegin, const wchar_t* pEnd) X_ABSTRACT;


	virtual size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const X_ABSTRACT;
	virtual size_t GetTextLength(const wchar_t* pStr, const bool asciiMultiLine) const X_ABSTRACT;

	// calculate the size.
	virtual Vec2f GetTextSize(const char* pStr, const XTextDrawConect& contex) X_ABSTRACT;
	virtual Vec2f GetTextSize(const wchar_t* pStr, const XTextDrawConect& contex) X_ABSTRACT;

	virtual uint32_t GetEffectId(const char* pEffectName) const X_ABSTRACT;

//	virtual void GetGradientTextureCoord(float& minU, float& minV,
//		float& maxU, float& maxV) const X_ABSTRACT;
};



X_NAMESPACE_END



#endif // !_X_FONT_I_H_
