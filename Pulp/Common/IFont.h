#pragma once

#ifndef _X_FONT_I_H_
#define _X_FONT_I_H_

struct ICore;

#ifdef IPFONT_EXPORTS
#define IPFONT_API DLL_EXPORT
#else
#define IPFONT_API DLL_IMPORT
#endif

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
	FRAMED
);

typedef Flags<DrawTextFlag> DrawTextFlags;

struct XTextDrawConect
{
	Vec2f	size;
	Rectf	clip;
	Colorf	col;
	float widthScale;
	bool proportinal;
	bool clipEnabled;
	bool drawFrame;
	bool scale800x600;
	uint32_t effectId;

	DrawTextFlags flags;
	IFont* pFont;

	XTextDrawConect() :
		proportinal(true),
		clipEnabled(false),
		drawFrame(false),
		scale800x600(false),
		widthScale(1.f),
		size(16.f,16.f),
		effectId(0)
	{}

	void Reset() { *this = XTextDrawConect(); }
	void SetSize(const Vec2f& _size) { size = _size; }
	void SetClip(const Rectf& rec) { clip = rec; }
	void SetProportional(bool proportional) { proportinal = proportional; }
	void EnableClipping(bool enable) { clipEnabled = enable; }
	void SetColor(const Colorf& _col) { col = _col; }
	void SetCharWidthScale(const float scale) { widthScale = scale; }
	void SetEffectId(uint32_t id) { effectId = id; }
	void SetDefaultEffect(void) { effectId = 0; }
	void SetDrawFrame(bool enable) { drawFrame = enable; }
	void SetScaleFrom800x600(bool enable) { scale800x600 = enable; }

	X_INLINE float GetCharWidth() const { return size.x; }
	X_INLINE float GetCharWidthScaled() const { return size.x * widthScale; }
	X_INLINE float GetCharHeight() const { return size.y; }
	X_INLINE float GetCharWidthScale() const { return widthScale; }
	X_INLINE uint32_t GetEffectId() const { return effectId; }
	X_INLINE bool getDrawFrame(void) const { return drawFrame; }
	X_INLINE bool getScaleFrom800x600(void) const { return scale800x600; }
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

//	virtual bool load(const char* pFilePath, uint32_t width, uint32_t height) X_ABSTRACT;

//	virtual bool loadTTF(const char* pFilePath, uint32_t width, uint32_t height) X_ABSTRACT;
	virtual bool loadFont(void) X_ABSTRACT;

	// expose drawing shit baby.
	virtual void DrawString(const Vec2f& pos, const char* pStr, const XTextDrawConect& contex) X_ABSTRACT;
	virtual void DrawString(float x, float y, const char* pStr, const XTextDrawConect& contex) X_ABSTRACT;
	virtual void DrawString(const Vec3f& pos, const char* pStr, const XTextDrawConect& contex) X_ABSTRACT;

	virtual void DrawStringW(const Vec2f& pos, const wchar_t* pStr, const XTextDrawConect& contex) X_ABSTRACT;
	virtual void DrawStringW(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& contex) X_ABSTRACT;


	virtual size_t GetTextLength(const char* pStr, const bool asciiMultiLine) const X_ABSTRACT;
	virtual size_t GetTextLengthW(const wchar_t* pStr, const bool asciiMultiLine) const X_ABSTRACT;

	// calculate the size.
	virtual Vec2f GetTextSize(const char* pStr, const XTextDrawConect& contex) X_ABSTRACT;
	virtual Vec2f GetTextSizeW(const wchar_t* pStr, const XTextDrawConect& contex) X_ABSTRACT;

	virtual uint32_t GetEffectId(const char* pEffectName) const X_ABSTRACT;

	virtual void GetGradientTextureCoord(float& minU, float& minV,
		float& maxU, float& maxV) const X_ABSTRACT;
};


struct IXFont_RenderProxy
{
	virtual ~IXFont_RenderProxy(){}
	virtual void RenderCallback(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& ctx) X_ABSTRACT;
};

X_NAMESPACE_END


#ifdef __cplusplus
extern "C"
{
#endif
	typedef X_NAMESPACE(font)::IFontSys(*IP_PTRCREATEFONTFNC(ICore *pCore));

	IPFONT_API X_NAMESPACE(font)::IFontSys *CreateFontInterface(ICore *pCore);

#ifdef __cplusplus
};
#endif


#endif // !_X_FONT_I_H_
