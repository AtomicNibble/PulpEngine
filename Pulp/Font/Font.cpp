#include "stdafx.h"
#include "Font.h"

#include "XFontTexture.h"

#include <IRender.h>
#include <ICore.h>
#include <ITexture.h>
#include <IFileSys.h>

#include <Math\VertexFormats.h>


X_NAMESPACE_BEGIN(font)

namespace
{

	Color8u g_ColorTable[10] = {
		Color8u(0, 0, 0),			// ^0 black
		Color8u(255, 0, 0),			// ^1 red
		Color8u(255, 240, 70),		// ^2 yellow
		Color8u(0, 0, 255),			// ^3 blue
		Color8u(0, 125, 255),		// ^4 light blue
		Color8u(255, 0, 220),		// ^5 pink
		Color8u(255, 145, 50),		// ^6 orange
		Color8u(255, 255, 255),		// ^7 white
		Color8u(0, 255, 0),			// ^8 green
		Color8u(255, 0, 255)
	};
} // namespace

const float XFFont::FONT_SPACE_SIZE = 0.2f;
const float XFFont::FONT_GLYPH_PROP_SPACING = 1.f;


XFFont::XFFont(ICore* pCore, XFont* pXFont, const char* pFontName) :
	pCore_(pCore),
	pXFont_(pXFont),
	name_(pFontName),
	pFontTexture_(nullptr),
	FontBuffer_(nullptr),
	texID_(-1),
	fontTexDirty_(false),
	pVertBuffer_(nullptr),
	effects_(g_fontArena)
{
	X_ASSERT_NOT_NULL(g_fontArena);

	pVertBuffer_ = X_NEW_ARRAY_ALIGNED( Vertex_P3F_T2F_C4B, FONT_QUAD_BUFFER_SIZE, 
		g_fontArena, "fontVertexBuffer", 16);
}


XFFont::~XFFont()
{
	Free();

	X_DELETE_ARRAY(pVertBuffer_, g_fontArena);
}

void XFFont::Release()
{
}


void XFFont::Free()
{
	effects_.clear();
	effects_.free();

	FreeBuffers();
	FreeTexture();
}

void XFFont::FreeBuffers()
{
	X_DELETE_AND_NULL(pFontTexture_, g_fontArena);
	X_DELETE_AND_NULL(FontBuffer_, g_fontArena);
}

void XFFont::FreeTexture()
{
	if (texID_ > -1) 
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pRender);

		gEnv->pRender->ReleaseTexture(texID_);

		texID_ = -1;
	}
}


bool XFFont::loadTTF(const char* pFilePath, uint32_t width, uint32_t height)
{
	core::Path<char> path;
	core::XFileScoped file;
	uint8_t* pBuffer = nullptr;
	size_t len = 0;

	uint32_t flags = 0;
	const int32_t iSmoothMethod = (flags & TTFFLAG_SMOOTH_MASK) >> TTFFLAG_SMOOTH_SHIFT;
	const int32_t iSmoothAmount = (flags & TTFFLAG_SMOOTH_AMOUNT_MASK) >> TTFFLAG_SMOOTH_AMOUNT_SHIFT;

	if (!pFilePath) {
		return false;
	}

	X_LOG0("Font", "loading: \"%s\"", pFilePath);

	path /= "Fonts/";
	path.setFileName(pFilePath);

	FreeBuffers();

	if (file.openFile(path.c_str(), core::fileModeFlags::READ))
	{
		len = safe_static_cast<size_t, uint64_t>(file.remainingBytes());
		if (len > 0) {
			pBuffer = X_NEW_ARRAY(uint8_t,len,g_fontArena, "FontTTFBuffer");
			file.read(pBuffer, len);
		}
	}

	if (pBuffer == nullptr || len == 0) {
		return false;
	}

	if (!pFontTexture_) {
		pFontTexture_ = X_NEW(XFontTexture, g_fontArena, "FontTexture")(g_fontArena);
	}

	if (!pFontTexture_->CreateFromMemory(pBuffer, len, width, height, iSmoothMethod, iSmoothAmount)) {

		X_DELETE_AND_NULL(pBuffer, g_fontArena);
		X_DELETE_AND_NULL(pFontTexture_, g_fontArena);

		return false;
	}

	FontBuffer_ = pBuffer;
	fontTexDirty_ = true;

	InitCache();

	return true;
}

void XFFont::Reload(void)
{
	// this is .shader reloading only for now.
	// thought a diffrent .tff file may be set.
	loadFont();
}

void XFFont::DrawString(const Vec2f& pos, const char* pStr, const XTextDrawConect& contex)
{
	if (!pStr) {
		return;
	}

	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	ByteToWide(pStr, strW, MAX_TXT_SIZE);

	DrawStringWInternal(Vec3f(pos.x, pos.y, 1.0f), strW, contex);
}


void XFFont::DrawString(float x, float y, const char* pStr, const XTextDrawConect& contex)
{
	if (!pStr) {
		return;
	}

	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	ByteToWide(pStr, strW, MAX_TXT_SIZE);

	DrawStringWInternal(Vec3f(x, y, 1.0f), strW, contex);
}


void XFFont::DrawString(const Vec3f& pos, const char* pStr, const XTextDrawConect& contex)
{
	if (!pStr) {
		return;
	}

	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	ByteToWide(pStr, strW, MAX_TXT_SIZE);

	DrawStringWInternal(pos, strW, contex);
}

// Wide
void XFFont::DrawStringW(const Vec2f& pos, const wchar_t* pStr, const XTextDrawConect& contex)
{
	DrawStringWInternal(Vec3f(pos.x,pos.y,1.0f), pStr, contex);
}

void XFFont::DrawStringW(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& contex)
{
	DrawStringWInternal(pos, pStr, contex);
}



// calculate the size.
Vec2f XFFont::GetTextSize(const char* pStr, const XTextDrawConect& contex)
{
	if (!pStr) {
		return Vec2f::zero();
	}

	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	ByteToWide(pStr, strW, MAX_TXT_SIZE);

	return GetTextSizeWInternal(strW, contex);
}

Vec2f XFFont::GetTextSizeW(const wchar_t* pStr, const XTextDrawConect& contex)
{
	return GetTextSizeWInternal(pStr, contex);
}

uint32_t XFFont::GetEffectId(const char* pEffectName) const
{
	X_ASSERT_NOT_NULL(pEffectName);
	Effets::ConstIterator it;
	uint32_t idx = 0;
	for (it = effects_.begin(); it != effects_.end(); ++it, idx++) {
		if (it->name.isEqual(pEffectName)) {
			break;
		}
	}
	return idx;
}


void XFFont::GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const
{
	const XTextureSlot* pSlot = pFontTexture_->GetGradientSlot();
	X_ASSERT_NOT_NULL(pSlot);

	const float invWidth = 1.0f / static_cast<float>(pFontTexture_->GetWidth());
	const float invHeight = 1.0f / static_cast<float>(pFontTexture_->GetHeight());

	// deflate by one pixel to avoid bilinear filtering on the borders
	minU = pSlot->texCoord[0] + invWidth;
	minV = pSlot->texCoord[1] + invHeight;
	maxU = pSlot->texCoord[0] + (pSlot->charWidth - 1) * invWidth;
	maxV = pSlot->texCoord[1] + (pSlot->charHeight - 1) * invHeight;
}

// =======================================================================================

void XFFont::DrawStringWInternal(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& contex)
{
	if (!pStr) {
		return;
	}

	render::IRender* pRenderer = gEnv->pRender;

	pRenderer->DrawStringW(this, pos, pStr, contex);
}

Vec2f XFFont::GetTextSizeWInternal(const wchar_t* pStr, const XTextDrawConect& ctx)
{
	X_PROFILE_BEGIN("FontTextSize", core::ProfileSubSys::FONT);

	if (!pStr) {
		return Vec2f::zero();
	}

	render::IRender* pRenderer = gEnv->pRender;

	float32_t maxW = 0;
	float32_t maxH = 0;

	Vec2f scale;
	float rcpCellWidth;

	// Scale it?
	Vec2f size = ctx.size; // in pixel
	if (ctx.getScaleFrom800x600())
	{
		pRenderer->ScaleCoord(size);
	}

	Prepare(pStr, false);


	if (ctx.proportinal)
	{
		rcpCellWidth = (1.0f / (512.0f / 16.0f)) * size.x;
		scale = Vec2f(rcpCellWidth, size.y / 32.0f);
	}
	else
	{
		rcpCellWidth = size.x / 16.0f;
		scale = Vec2f(rcpCellWidth * ctx.widthScale, size.y * ctx.widthScale / 16.0f);
	}


	float charX = 0;
	float charY = 0 + size.y;

	if (charY > maxH) {
		maxH = charY;
	}

	// parse the string, ignoring control characters
	wchar_t* pChar = (wchar_t*)pStr;
	while (wchar_t ch = *pChar++)
	{
		switch (ch)
		{
			case L'\\':
			{
				if (*pChar != L'n')
					break;

				++pChar;
			}
			case L'\n':
			{
				if (charX > maxW)
					maxW = charX;

				charX = 0;
				charY += size.y;

				if (charY > maxH)
					maxH = charY;

				continue;
			}
				break;
			case L'\r':
			{
				if (charX > maxW)
					 maxW = charX;

				charX = 0;
				continue;
			}
				break;
			case L'\t':
			{
				if (ctx.proportinal)
					charX += FONT_TAB_CHAR_NUM * size.x * FONT_SPACE_SIZE;
				else
					charX += FONT_TAB_CHAR_NUM * size.x * ctx.widthScale;
				continue;
			}
				break;
			case L' ':
			{
				 if (ctx.proportinal)
					charX += FONT_SPACE_SIZE * size.x;
				 else
					 charX += size.x * ctx.widthScale;
				continue;
			}
				break;
			case L'^':
			{
				if (*pChar == L'^')
					++pChar;
				 else if (*pChar)
				{
					++pChar;
					continue;
				}
			}
				break;
			default:
				break;
		}

		float advance;
		if (ctx.proportinal)
		{
			int iCharWidth = this->pFontTexture_->GetCharacterWidth(ch);
			advance = (iCharWidth + FONT_GLYPH_PROP_SPACING) * scale.x;
		}
		else
		{
			advance = size.x * ctx.widthScale;
		}

		charX += advance;
	}

	if (charX > maxW) {
		maxW = charX;
	}

	return Vec2f(maxW, maxH);
}

size_t XFFont::GetTextLength(const char* pStr, const bool asciiMultiLine) const
{
	size_t size = 0;

	// parse the string, ignoring control characters
	const char* pChar = pStr;
	while (char ch = *pChar++)
	{
		switch (ch)
		{
			case '\\':
			{
				if (*pChar != 'n' || !asciiMultiLine)
				{
					break;
				}
				++pChar;
			}
			case '\n':
			case '\r':
			case '\t':
			{
				continue;
			}
				break;
			case '^':
			{
				if (*pChar == '^')
				{
					++pChar;
				}
				else if (*pChar)
				{
					++pChar;
					continue;
				}
			}
				break;
			default:
				break;
		}
		++size;
	}

	return size;
}

size_t XFFont::GetTextLengthW(const wchar_t* pStr, const bool asciiMultiLine) const
{
	size_t size = 0;

	// parse the string, ignoring control characters
	const wchar_t* pChar = pStr;
	while (wchar_t ch = *pChar++)
	{
		switch (ch)
		{
			case L'\\':
			{
				if (*pChar != L'n' || !asciiMultiLine)
				{
					break;
				}
				++pChar;
			}
			case L'\n':
			case L'\r':
			case L'\t':
			{
				continue;
			}
				break;
			case L'$':
			{
				if (*pChar == L'$')
				{
					++pChar;
				}
				else if (*pChar)
				{
					++pChar;
					continue;
				}
			}
				break;
			default:
				break;
		}
		++size;
	}

	return size;
}

bool XFFont::InitCache(void)
{
	pFontTexture_->CreateGradientSlot();

	// precache (not required but for faster printout later)
	wchar_t buf[256];
	wchar_t* p = buf;

	wchar_t i = L' '; // even space should be there - otherwise it's created later on demand

	// precache all [normal] printable characters to the string (missing once are updated on demand)
	for (; i <= L'~'; ++i) {
		*p++ = i;
	}

	i += 35;

	for (; i < 256; ++i) {
		*p++ = i;
	}

	*p = 0;

	Prepare(buf, false);

	return true;
}

void XFFont::Prepare(const wchar_t* pStr, bool updateTexture)
{
	X_PROFILE_BEGIN("FontPrepare", core::ProfileSubSys::FONT);

	bool cache = pFontTexture_->PreCacheString(pStr) == 1;
	bool texUpdateNeeded = fontTexDirty_ || cache;
	if (updateTexture && texUpdateNeeded && texID_ >= 0)
	{
		gEnv->pRender->FontUpdateTexture(texID_, 0, 0, 
			pFontTexture_->GetWidth(),
			pFontTexture_->GetHeight(),
			pFontTexture_->GetBuffer()
		);

		fontTexDirty_ = false;
	}
	else
	{
		fontTexDirty_ = texUpdateNeeded;
	}
}


void XFFont::RenderCallback(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& ctx)
{
	X_PROFILE_BEGIN("FontRender", core::ProfileSubSys::FONT);

	Vertex_P3F_T2F_C4B* pQuad, *pVertex;
	XTextureSlot* pSlot;
	render::IRender* pRenderer;
	XCharCords cords;
	int charWidth;
	uint32_t vbOffset;
	bool drawFrame;

	if (texID_ == -1 && !InitTexture(), 0) {
		return;
	}

	if (effects_.isEmpty()) {
		X_WARNING("Font", "%s has no effects.", this->getName());
		return;
	}

	vbOffset = 0;
	pRenderer = gEnv->pRender;

	Prepare(pStr, true);

	pRenderer->FontSetTexture(texID_);
	if (!pRenderer->FontSetRenderingState()) {
		return;
	}

	// Scale it?
	Vec2f size = ctx.size; // in pixel
	if (ctx.getScaleFrom800x600())
	{
		pRenderer->ScaleCoord(size);
	}
	
	
	// loop over the pases you silly slut !
	size_t passIdx, effecIdx;

	effecIdx = ctx.GetEffectId() < effects_.size() ? ctx.GetEffectId() : 0;
	drawFrame = ctx.getDrawFrame();

	FontEffect& effect = effects_[effecIdx];
	for (passIdx = 0; passIdx < effect.passes.size(); passIdx++)
	{
		const FontPass* pass = &effect.passes[passIdx];

		Color8u col = ctx.col;

		float z = pos.z;
		float charX = pos.x + pass->offset.x;
		float charY = pos.y + pass->offset.y;
		float rcpCellWidth;

		Vec2f baseXY(pos.x, pos.y);
		Vec2f scale;

		if (ctx.proportinal)
		{
			// to have backward compatible behavior (can be refactored later)
			rcpCellWidth = (1.0f / (512.0f / 16.0f)) * size.x;
			scale = Vec2f(rcpCellWidth * ctx.widthScale, size.y / 32.0f);
		}
		else
		{
			rcpCellWidth = size.x / 16.0f;
			scale = Vec2f(rcpCellWidth * ctx.widthScale, size.y * ctx.widthScale / 16.0f);
		}

		if (drawFrame && passIdx == 0)
		{
			Vec2f textSize = GetTextSizeWInternal(pStr, ctx);

			Color8u frameColor(7,7,7,80); //dark grey, 65% opacity


			float x0 = baseXY.x - 3;
			float y0 = baseXY.y;
			float x1 = baseXY.x + textSize.x + 2;
			float y1 = baseXY.y + textSize.y;

			Vec3f v0(x0, y0, z);
			Vec3f v2(x1, y1, z);
			Vec3f v1(v2.x, v0.y, v0.z); // to avoid float->half conversion
			Vec3f v3(v0.x, v2.y, v0.z); // to avoid float->half conversion


			pVertex = &pVertBuffer_[vbOffset];

			Vec2f gradientUvMin, gradientUvMax;
			GetGradientTextureCoord(gradientUvMin.x, gradientUvMin.y, gradientUvMax.x, gradientUvMax.y);

			// define the frame quad
			pVertex[0].pos = v0;
			pVertex[0].color = frameColor;
			pVertex[0].st = Vec2f(gradientUvMin.x, gradientUvMax.y);

			pVertex[1].pos = v1;
			pVertex[1].color = frameColor;
			pVertex[1].st = pVertex[0].st;

			pVertex[2].pos = v2;
			pVertex[2].color = frameColor;
			pVertex[2].st = pVertex[0].st;

			pVertex[3].pos = v2;
			pVertex[3].color = frameColor;
			pVertex[3].st = pVertex[0].st;

			pVertex[4].pos = v3;
			pVertex[4].color = frameColor;
			pVertex[4].st = pVertex[0].st;

			pVertex[5].pos = v0;
			pVertex[5].color = frameColor;
			pVertex[5].st = pVertex[0].st;

			vbOffset = 6;

		}

		const wchar_t* pChar = (const wchar_t*)pStr;
		while (wchar_t ch = *pChar++)
		{
			switch (ch)
			{
				case L' ':
				{
							 if (ctx.proportinal)
								 charX += size.x * FONT_SPACE_SIZE;
							 else
								 charX += size.x * ctx.widthScale;
							 continue;
							 break;
				}

				case L'^':
				{
							 if (*pChar == L'^')
								 ++pChar;
							 else if (core::strUtil::IsDigitW(*pChar))
							 {
								 int colorIndex = (*pChar) - L'0';
								 Color8u newColor = g_ColorTable[colorIndex];
								 col.r = newColor.r;
								 col.g = newColor.g;
								 col.b = newColor.b;
								 ++pChar;
							 }
							 continue;
							 break;
				}

				case L'\n':
				{
							  charX = baseXY.x;
							  charY += size.y;
							  continue;
							  break;
				}

				case L'\r':
				{
							  charX = baseXY.x;
							  continue;
							  break;
				}

				case L'\t':
				{
							  if (ctx.proportinal)
								  charX += FONT_TAB_CHAR_NUM * size.x * FONT_SPACE_SIZE;
							  else
								  charX += FONT_TAB_CHAR_NUM * size.x * ctx.widthScale;
							  continue;
							  break;
				}

				default:
					break;
			}

			charWidth = pFontTexture_->GetCharacterWidth(ch);
			pSlot = pFontTexture_->GetCharSlot(ch);
			pFontTexture_->GetTextureCoord(pSlot, cords);
			pQuad = &pVertBuffer_[vbOffset];

			float advance;

			if (ctx.proportinal)
			{
				advance = (charWidth + FONT_GLYPH_PROP_SPACING) * scale.x;
				cords.offset.x = 0;
			}
			else
			{
				advance = size.x * ctx.widthScale;
			}

			float xpos = charX + cords.offset.x * scale.x;
			float ypos = charY + cords.offset.y * scale.y;
			float right = xpos + cords.size.x * scale.x;
			float bottom = ypos + cords.size.y * scale.y;

			// Verts
			Vec3f tl(xpos, ypos, z);
			Vec3f br(right, bottom, z);
			Vec3f tr(br.x, tl.y, tl.z);
			Vec3f bl(tl.x, br.y, tl.z);

			// cords
			Vec2f tc_tl(cords.texCoords[0], cords.texCoords[1]);
			Vec2f tc_br(cords.texCoords[2], cords.texCoords[3]);
			Vec2f tc_tr(tc_br.x, tc_tl.y);
			Vec2f tc_bl(tc_tl.x, tc_br.y);

			Color8u finalCol = pass->col * col;
			// We need 6 since each char is not connected.
			// so we make seprate tri's

			// TL
			pQuad[0].pos = tl;
			pQuad[0].color = finalCol;
			pQuad[0].st = tc_tl;

			// TR
			pQuad[1].pos = tr;
			pQuad[1].color = finalCol;
			pQuad[1].st = tc_tr;

			// BR
			pQuad[2].pos = br;
			pQuad[2].color = finalCol;
			pQuad[2].st = tc_br;

			// BR
			pQuad[3].pos = br;
			pQuad[3].color = finalCol;
			pQuad[3].st = tc_br;

			// BL
			pQuad[4].pos = bl;
			pQuad[4].color = finalCol;
			pQuad[4].st = tc_bl;

			// TL
			pQuad[5].pos = tl;
			pQuad[5].color = finalCol;
			pQuad[5].st = tc_tl;


			vbOffset += 6;

			if (vbOffset == FONT_QUAD_BUFFER_SIZE)
			{
				X_ASSERT_NOT_IMPLEMENTED();
			//	pRenderer->DrawVB(pVertBuffer_, vbOffset,
			//		render::PrimitiveTypePublic::TriangleList);

				// reset baby.
				vbOffset = 0;
			}

			charX += advance;
		}
	}


	// draw anything left.
	if (vbOffset) {
		X_ASSERT_NOT_IMPLEMENTED();
	//	pRenderer->DrawVB(pVertBuffer_, vbOffset,
	//		render::PrimitiveTypePublic::TriangleList);
	}

	pRenderer->FontRestoreRenderingState();
}


bool XFFont::InitTexture(void)
{
	texID_ = gEnv->pRender->FontCreateTexture(
		pFontTexture_->GetSize(), 
		pFontTexture_->GetBuffer(), 
		texture::Texturefmt::A8);

	if (texID_ <= 0) {
		X_WARNING("Font", "Failed to create font texture.");
	}

	return texID_ >= 0;
}


void XFFont::ByteToWide(const char* pStr, wchar_t* pOut, const size_t buflen)
{
	const size_t len = core::Min<size_t>(buflen, strlen(pStr));

	for (size_t i = 0; i < len; ++i) {
		pOut[i] = static_cast<uint8_t>(pStr[i]);
	}
	pOut[len] = 0;
}

X_NAMESPACE_END
