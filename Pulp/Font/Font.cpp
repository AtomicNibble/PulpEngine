#include "stdafx.h"
#include "Font.h"

#include "XFontTexture.h"

#include <IRender.h>
#include <ICore.h>
#include <ITexture.h>
#include <IFileSys.h>
#include <IPrimativeContext.h>

#include <Math\VertexFormats.h>

#include <Util\ScopedPointer.h>


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

const float XFont::FONT_SPACE_SIZE = 0.2f;
const float XFont::FONT_GLYPH_PROP_SPACING = 1.f;


XFont::XFont(ICore* pCore, XFontSystem* pFontSys, const char* pFontName) :
	pCore_(pCore),
	pFontSys_(pFontSys),
	name_(pFontName),
	pFontTexture_(nullptr),
	pTexture_(nullptr),
	fontTexDirty_(false),
	effects_(g_fontArena)
{
	X_ASSERT_NOT_NULL(g_fontArena);
}


XFont::~XFont()
{
	Free();

}

void XFont::Release()
{
}


void XFont::Free()
{
	effects_.clear();
	effects_.free();

	FreeBuffers();
	FreeTexture();
}

void XFont::FreeBuffers()
{
	X_DELETE_AND_NULL(pFontTexture_, g_fontArena);
}

void XFont::FreeTexture()
{
	if (pTexture_)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pRender);

		gEnv->pRender->releaseTexture(pTexture_);
		pTexture_ = nullptr;
	}
}


bool XFont::loadTTF(const char* pFilePath, uint32_t width, uint32_t height)
{

	uint32_t flags = 0;
	const int32_t iSmoothMethod = (flags & TTFFLAG_SMOOTH_MASK) >> TTFFLAG_SMOOTH_SHIFT;
	const int32_t iSmoothAmount = (flags & TTFFLAG_SMOOTH_AMOUNT_MASK) >> TTFFLAG_SMOOTH_AMOUNT_SHIFT;

	if (!pFilePath) {
		return false;
	}

	X_LOG0("Font", "loading: \"%s\"", pFilePath);

	core::Path<char> path;
	path /= "Fonts/";
	path.setFileName(pFilePath);

	FreeBuffers();

	core::ScopedPointer<uint8_t[]> fileDataBuf(g_fontArena, nullptr);

	size_t len = 0;

	core::XFileScoped file;
	if (file.openFile(path.c_str(), core::fileModeFlags::READ))
	{
		len = safe_static_cast<size_t, uint64_t>(file.remainingBytes());
		if (len > 0) {
			fileDataBuf.reset(X_NEW_ARRAY(uint8_t,len, fileDataBuf.getArena(), "FontTTFBuffer"));
			if (file.read(fileDataBuf.get(), len) != len) {

			}
		}
	}

	if (fileDataBuf.get() == nullptr || len == 0) {
		return false;
	}

	if (!pFontTexture_) {
		pFontTexture_ = X_NEW(XFontTexture, g_fontArena, "FontTexture")(g_fontArena);
	}

	if (!pFontTexture_->CreateFromMemory(fileDataBuf.get(), len, width, height, iSmoothMethod, iSmoothAmount)) {
		X_DELETE_AND_NULL(pFontTexture_, g_fontArena);
		return false;
	}

	fontTexDirty_ = true;

	InitCache();

	return true;
}

void XFont::Reload(void)
{
	// this is .shader reloading only for now.
	// thought a diffrent .tff file may be set.
	loadFont();
}



void XFont::DrawString(engine::IPrimativeContext* pPrimCon, render::StateHandle stateHandle, const Vec3f& pos,
	const XTextDrawConect& contex, const char* pBegin, const char* pEnd)
{
	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	
	size_t txtLen = pEnd - pBegin;
	const wchar_t* pWideBegin = core::strUtil::Convert(pBegin, strW);
	const wchar_t* pWideEnd = pWideBegin + txtLen;

	DrawString(
		pPrimCon,
		stateHandle,
		pos,
		contex,
		pWideBegin,
		pWideEnd
	);
}

void XFont::DrawString(engine::IPrimativeContext* pPrimCon, render::StateHandle stateHandle, const Vec3f& pos,
	const XTextDrawConect& ctx, const wchar_t* pBegin, const wchar_t* pEnd)
{
	X_UNUSED(pEnd);

	if (!pTexture_ && !InitTexture()) {
		return;
	}

	Vec2f size = ctx.size; // in pixel
	const bool proportinal = ctx.flags.IsSet(DrawTextFlag::FIXED_SIZE);
	const bool drawFrame = ctx.flags.IsSet(DrawTextFlag::FRAMED);

	const auto effecIdx = ctx.GetEffectId() < effects_.size() ? ctx.GetEffectId() : 0;
	const auto textId = pTexture_->getTexID();

	FontEffect& effect = effects_[effecIdx];
	for (auto passIdx = 0u; passIdx < effect.passes.size(); passIdx++)
	{
		const FontPass& pass = effect.passes[passIdx];

		Color8u col = ctx.col;

		float z = pos.z;
		float charX = pos.x + pass.offset.x;
		float charY = pos.y + pass.offset.y;
		float rcpCellWidth;

		Vec2f baseXY(pos.x, pos.y);
		Vec2f scale;

		if (proportinal)
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
			const Vec2f textSize = GetTextSizeWInternal(pBegin, ctx);

			const Color8u frameColor(7, 7, 7, 80); //dark grey, 65% opacity

			const float x0 = baseXY.x - 3;
			const float y0 = baseXY.y;
			const float x1 = baseXY.x + textSize.x + 2;
			const float y1 = baseXY.y + textSize.y;

			const Vec3f v0(x0, y0, z);
			const Vec3f v2(x1, y1, z);
			const Vec3f v1(v2.x, v0.y, v0.z); // to avoid float->half conversion
			const Vec3f v3(v0.x, v2.y, v0.z); // to avoid float->half conversion

			Vec2f gradientUvMin, gradientUvMax;
			GetGradientTextureCoord(gradientUvMin.x, gradientUvMin.y, gradientUvMax.x, gradientUvMax.y);

			Vertex_P3F_T2F_C4B* pVerts = pPrimCon->addPrimative(6, render::TopoType::TRIANGLELIST, textId, stateHandle);

			pVerts[0].pos = v0;
			pVerts[0].color = frameColor;
			pVerts[0].st = Vec2f(gradientUvMin.x, gradientUvMax.y);

			pVerts[1].pos = v1;
			pVerts[1].color = frameColor;
			pVerts[1].st = pVerts[0].st;

			pVerts[2].pos = v2;
			pVerts[2].color = frameColor;
			pVerts[2].st = pVerts[0].st;

			pVerts[3].pos = v2;
			pVerts[3].color = frameColor;
			pVerts[3].st = pVerts[0].st;

			pVerts[4].pos = v3;
			pVerts[4].color = frameColor;
			pVerts[4].st = pVerts[0].st;

			pVerts[5].pos = v0;
			pVerts[5].color = frameColor;
			pVerts[5].st = pVerts[0].st;
		}

		const wchar_t* pChar = pBegin;
		while (wchar_t ch = *pChar++)
		{
			// check for special chars that alter how we render.
			switch (ch)
			{
				case L' ':
				{
					if (proportinal) {
						charX += size.x * FONT_SPACE_SIZE;
					}
					else {
						charX += size.x * ctx.widthScale;
					}
					continue;
					break;
				}

				case L'^':
				{
					if (*pChar == L'^') {
						++pChar;
					}
					else if (core::strUtil::IsDigitW(*pChar))
					{
						const int32_t colorIndex = (*pChar) - L'0';
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
					if (proportinal) {
						charX += FONT_TAB_CHAR_NUM * size.x * FONT_SPACE_SIZE;
					}
					else {
						charX += FONT_TAB_CHAR_NUM * size.x * ctx.widthScale;
					}
					continue;
					break;
				}

				default:
					break;
			}

			const int32_t charWidth = pFontTexture_->GetCharacterWidth(ch);
			const XTextureSlot* pSlot = pFontTexture_->GetCharSlot(ch);
			XCharCords cords;
			pFontTexture_->GetTextureCoord(pSlot, cords);
			
			Vertex_P3F_T2F_C4B* pVerts = pPrimCon->addPrimative(6, render::TopoType::TRIANGLELIST, textId, stateHandle);

			float hozAdvance = 0;
			if (proportinal)
			{
				hozAdvance = (charWidth + FONT_GLYPH_PROP_SPACING) * scale.x;
				cords.offset.x = 0;
			}
			else
			{
				hozAdvance = size.x * ctx.widthScale;
			}

			float xpos = charX + cords.offset.x * scale.x;
			float ypos = charY + cords.offset.y * scale.y;
			float right = xpos + cords.size.x * scale.x;
			float bottom = ypos + cords.size.y * scale.y;

			// Verts
			const Vec3f tl(xpos, ypos, z);
			const Vec3f br(right, bottom, z);
			const Vec3f tr(br.x, tl.y, tl.z);
			const Vec3f bl(tl.x, br.y, tl.z);

			// cords
			const Vec2f tc_tl(cords.texCoords[0], cords.texCoords[1]);
			const Vec2f tc_br(cords.texCoords[2], cords.texCoords[3]);
			const Vec2f tc_tr(tc_br.x, tc_tl.y);
			const Vec2f tc_bl(tc_tl.x, tc_br.y);

			const Color8u finalCol = pass.col * col;
			// We need 6 since each char is not connected.
			// so we make seprate tri's

			// TL
			pVerts[0].pos = tl;
			pVerts[0].color = finalCol;
			pVerts[0].st = tc_tl;

			// TR
			pVerts[1].pos = tr;
			pVerts[1].color = finalCol;
			pVerts[1].st = tc_tr;

			// BR
			pVerts[2].pos = br;
			pVerts[2].color = finalCol;
			pVerts[2].st = tc_br;

			// BR
			pVerts[3].pos = br;
			pVerts[3].color = finalCol;
			pVerts[3].st = tc_br;

			// BL
			pVerts[4].pos = bl;
			pVerts[4].color = finalCol;
			pVerts[4].st = tc_bl;

			// TL
			pVerts[5].pos = tl;
			pVerts[5].color = finalCol;
			pVerts[5].st = tc_tl;

			charX += hozAdvance;
		}
	}

}


size_t XFont::GetTextLength(const char* pStr, const bool asciiMultiLine) const
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

size_t XFont::GetTextLength(const wchar_t* pStr, const bool asciiMultiLine) const
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


// calculate the size.
Vec2f XFont::GetTextSize(const char* pStr, const XTextDrawConect& contex)
{
	if (!pStr) {
		return Vec2f::zero();
	}

	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	ByteToWide(pStr, strW, MAX_TXT_SIZE);

	return GetTextSizeWInternal(strW, contex);
}

Vec2f XFont::GetTextSize(const wchar_t* pStr, const XTextDrawConect& contex)
{
	return GetTextSizeWInternal(pStr, contex);
}

uint32_t XFont::GetEffectId(const char* pEffectName) const
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


void XFont::GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const
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


Vec2f XFont::GetTextSizeWInternal(const wchar_t* pStr, const XTextDrawConect& ctx)
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
	if (ctx.flags.IsSet(DrawTextFlag::SCALE_800x600))
	{
		pRenderer->ScaleCoord(size);
	}

	Prepare(pStr, false);

	const bool proportinal = ctx.flags.IsSet(DrawTextFlag::FIXED_SIZE);

	if (proportinal)
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
				if (proportinal)
					charX += FONT_TAB_CHAR_NUM * size.x * FONT_SPACE_SIZE;
				else
					charX += FONT_TAB_CHAR_NUM * size.x * ctx.widthScale;
				continue;
			}
				break;
			case L' ':
			{
				 if (proportinal)
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
		if (proportinal)
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

bool XFont::InitCache(void)
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

void XFont::Prepare(const wchar_t* pStr, bool updateTexture)
{
	X_PROFILE_BEGIN("FontPrepare", core::ProfileSubSys::FONT);

	const bool cache = pFontTexture_->PreCacheString(pStr) == 1;
	const bool texUpdateNeeded = fontTexDirty_ || cache;

	if (updateTexture && texUpdateNeeded && pTexture_)
	{
	//	gEnv->pRender->FontUpdateTexture(texID_, 0, 0, 
	//		pFontTexture_->GetWidth(),
	//		pFontTexture_->GetHeight(),
	//		pFontTexture_->GetBuffer()
	//	);

		fontTexDirty_ = false;
	}
	else
	{
		fontTexDirty_ = texUpdateNeeded;
	}
}

#if 0
void XFont::RenderCallback(const Vec3f& pos, const wchar_t* pStr, const XTextDrawConect& ctx)
{
	X_PROFILE_BEGIN("FontRender", core::ProfileSubSys::FONT);

	Vertex_P3F_T2F_C4B* pVerts, *pVertex;
	XTextureSlot* pSlot;
//	render::IRender* pRenderer;
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
//	pRenderer = gEnv->pRender;

	Prepare(pStr, true);

//	pRenderer->FontSetTexture(texID_);
//	if (!pRenderer->FontSetRenderingState()) {
//		return;
//	}

	// Scale it?
	Vec2f size = ctx.size; // in pixel
	if (ctx.getScaleFrom800x600())
	{
	//	pRenderer->ScaleCoord(size);
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
			pVerts = &pVertBuffer_[vbOffset];

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
			pVerts[0].pos = tl;
			pVerts[0].color = finalCol;
			pVerts[0].st = tc_tl;

			// TR
			pVerts[1].pos = tr;
			pVerts[1].color = finalCol;
			pVerts[1].st = tc_tr;

			// BR
			pVerts[2].pos = br;
			pVerts[2].color = finalCol;
			pVerts[2].st = tc_br;

			// BR
			pVerts[3].pos = br;
			pVerts[3].color = finalCol;
			pVerts[3].st = tc_br;

			// BL
			pVerts[4].pos = bl;
			pVerts[4].color = finalCol;
			pVerts[4].st = tc_bl;

			// TL
			pVerts[5].pos = tl;
			pVerts[5].color = finalCol;
			pVerts[5].st = tc_tl;


			vbOffset += 6;

//			if (vbOffset == FONT_QUAD_BUFFER_SIZE)
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


}
#endif


bool XFont::InitTexture(void)
{
	pTexture_ = gEnv->pRender->createTexture(
		"$fontTexture",
		pFontTexture_->GetSize(), 
		texture::Texturefmt::A8,
		pFontTexture_->GetBuffer()
	);

	if (!pTexture_->isLoaded()) {
		X_WARNING("Font", "Failed to create font texture.");
		return false;
	}

	return true;
}


void XFont::ByteToWide(const char* pStr, wchar_t* pOut, const size_t buflen)
{
	const size_t len = core::Min<size_t>(buflen, strlen(pStr));

	for (size_t i = 0; i < len; ++i) {
		pOut[i] = static_cast<uint8_t>(pStr[i]);
	}
	pOut[len] = 0;
}

X_NAMESPACE_END
