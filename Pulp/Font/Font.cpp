#include "stdafx.h"
#include "Font.h"

#include "XFontTexture.h"
#include "XFontSystem.h"

#include <IRender.h>
#include <I3DEngine.h>
#include <ICore.h>
#include <ITexture.h>
#include <IMaterial.h>
#include <IFileSys.h>
#include <IPrimativeContext.h>
#include <IRenderCommands.h>

#include <Math\VertexFormats.h>

#include <../../tools/MaterialLib/MatLib.h>

#include "CmdBucket.h"

X_NAMESPACE_BEGIN(font)

namespace
{

	Color8u g_ColorTable[10] = {
		Color8u(0, 0, 0),			// ^0 black
		Color8u(255, 0, 0),			// ^1 red
		Color8u(255, 240, 70),		// ^2 yellow
		Color8u(0, 0, 255),			// ^3 blue
		Color8u(0, 125, 255),		// ^4 light blue
	//	Color8u(255, 0, 220),		// ^5 pink
		Color8u(255, 170, 155),		// ^5 pink
		Color8u(255, 145, 50),		// ^6 orange
		Color8u(255, 255, 255),		// ^7 white
		Color8u(0, 255, 0),			// ^8 green
		Color8u(255, 0, 255)
	};
} // namespace


XFont::XFont(XFontSystem& fontSys, const char* pFontName) :
	fontSys_(fontSys),
	name_(pFontName),
	pFontTexture_(nullptr),
	effects_(g_fontArena),
	pTexture_(nullptr),
	fontTexDirty_(false),
	pMaterial_(nullptr),
	signal_(false),
	loadStatus_(LoadStatus::NotLoaded)
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


void XFont::Free(void)
{
	effects_.clear();
	effects_.free();

	FreeBuffers();
	FreeTexture();
}

void XFont::FreeBuffers(void)
{
	fontSys_.releaseFontTexture(pFontTexture_);
	pFontTexture_ = nullptr;
	fontTexDirty_ = true;
}

void XFont::FreeTexture(void)
{
	if (pTexture_)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pRender);

		gEnv->pRender->releaseTexture(pTexture_);
		pTexture_ = nullptr;
	}
}

bool XFont::isReady(void)
{
	return (pFontTexture_ && pFontTexture_->IsReady());
}

bool XFont::WaitTillReady(void)
{
	if (isReady()) {
		return true;
	}

	// if we don't have a font texture :S
	// how do we wait!
	if (!pFontTexture_) 
	{
		while (loadStatus_ == LoadStatus::Loading)
		{
			// if we have job system try help with work.
			// if no work wait...
			if (!gEnv->pJobSys || !gEnv->pJobSys->HelpWithWork())
			{
				signal_.wait();
			}
		}

		signal_.clear();

		if (loadStatus_ == LoadStatus::Error || loadStatus_ == LoadStatus::NotLoaded) {
			return false;
		}

		X_ASSERT_NOT_NULL(pFontTexture_);
	}

	return pFontTexture_->WaitTillReady();
}


void XFont::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
	const XTextDrawConect& contex, const char* pBegin, const char* pEnd)
{
	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	
	size_t txtLen = pEnd - pBegin;
	const wchar_t* pWideBegin = core::strUtil::Convert(pBegin, strW);
	const wchar_t* pWideEnd = pWideBegin + txtLen;

	DrawString(
		pPrimCon,
		pos,
		contex,
		pWideBegin,
		pWideEnd
	);
}

void XFont::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
	const XTextDrawConect& ctx, const wchar_t* pBegin, const wchar_t* pEnd)
{
	const size_t textLen = (pEnd - pBegin);
	if (!textLen) {
		return;
	}

	// we may be async loading so must wait till we are ready.
	if (!isReady()) {
		return;
	}

	if (!pMaterial_ && !CreateDeviceTexture()) {
		return;
	}

	if (effects_.isEmpty()) {
		X_WARNING("Font", "\"%s\" has no effects.", getName());
		return;
	}

	X_ASSERT(ctx.GetEffectId() >= 0 && ctx.GetEffectId() < static_cast<int32_t>(effects_.size()), 
		"Effect index invalid")(ctx.GetEffectId(), effects_.size());

	// updates LRU cache and adds glyphs to font texture if missing.
	// the device texture is not updated here tho.
	Prepare(pBegin, pEnd);


	Vec2f size = ctx.size; // in pixel
	const bool proportinal = ctx.flags.IsSet(DrawTextFlag::FIXED_SIZE);
	const bool drawFrame = ctx.flags.IsSet(DrawTextFlag::FRAMED);

	const auto effecIdx = ctx.GetEffectId();

#if 0
	X_UNUSED(pos);
	X_UNUSED(pPrimCon);

#else
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
			const Vec2f textSize = GetTextSizeWInternal(pBegin, pEnd, ctx);

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

			engine::IPrimativeContext::PrimVertex* pVerts = pPrimCon->addPrimative(6, render::TopoType::TRIANGLELIST, pMaterial_);

			pVerts[0].pos = v0;
			pVerts[0].color = frameColor;
			pVerts[0].st = core::XHalf2::compress(gradientUvMin.x, gradientUvMax.y);

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
			
			engine::IPrimativeContext::PrimVertex* pVerts = pPrimCon->addPrimative(6, render::TopoType::TRIANGLELIST, pMaterial_);

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
			const core::XHalf2 tc_tl(cords.texCoords[0], cords.texCoords[1]);
			const core::XHalf2 tc_br(cords.texCoords[2], cords.texCoords[3]);
			const core::XHalf2 tc_tr(tc_br.x, tc_tl.y);
			const core::XHalf2 tc_bl(tc_tl.x, tc_br.y);

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
#endif
}


size_t XFont::GetTextLength(const char* pBegin, const char* pEnd, const bool asciiMultiLine) const
{
	size_t size = 0;

	// parse the string, ignoring control characters
	const char* pCur = pBegin;
	while (pCur < pEnd)
	{
		switch (*pCur++)
		{
			case '\\':
			{
				if (*pCur != 'n' || !asciiMultiLine)
				{
					break;
				}
				++pCur;
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
				if (*pCur == '^')
				{
					++pCur;
				}
				else if (*pCur)
				{
					++pCur;
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

size_t XFont::GetTextLength(const wchar_t* pBegin, const wchar_t* pEnd, const bool asciiMultiLine) const
{
	size_t size = 0;

	// parse the string, ignoring control characters
	const wchar_t* pCur = pBegin;
	while (pCur < pEnd)
	{
		switch (*pCur++)
		{
			case L'\\':
			{
				if (*pCur != L'n' || !asciiMultiLine)
				{
					break;
				}
				++pCur;
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
				if (*pCur == L'$')
				{
					++pCur;
				}
				else if (*pCur)
				{
					++pCur;
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
Vec2f XFont::GetTextSize(const char* pBegin, const char* pEnd, const XTextDrawConect& contex)
{
	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	const size_t len = ByteToWide(pBegin, pEnd, strW, MAX_TXT_SIZE);

	return GetTextSizeWInternal(strW, strW + len, contex);
}

Vec2f XFont::GetTextSize(const wchar_t* pBegin, const wchar_t* pEnd, const XTextDrawConect& contex)
{
	return GetTextSizeWInternal(pBegin, pEnd, contex);
}

int32_t XFont::GetEffectId(const char* pEffectName) const
{
	X_ASSERT_NOT_NULL(pEffectName);

	int32_t idx = 0;
	for (; idx < static_cast<int32_t>(effects_.size()); ++idx) {
		if (effects_[idx].name.isEqual(pEffectName)) {
			break;
		}
	}

	if (idx == static_cast<int32_t>(effects_.size())) {
		X_ERROR("Font", "Failed to find effect with name: \"%s\"", pEffectName);
		return -1;
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

void XFont::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket)
{
	if (!isDirty()) {
		return;
	}

	// we may of created the texture and done the initally precache
	// but never actually drawn anything with the font.
	// so currently no point in uploading a texture.
	if (!pTexture_) {
		return;
	}

	const auto& buf = pFontTexture_->GetBuffer();

	// we want to submit a draw
	render::Commands::CopyTextureBufferData* pCopyCmd = bucket.addCommand<render::Commands::CopyTextureBufferData>(0, 0);
	pCopyCmd->textureId = pTexture_->getTexID();
	pCopyCmd->pData = buf.data();
	pCopyCmd->size = safe_static_cast<uint32_t>(buf.size());

	fontTexDirty_ = false;
}

// =======================================================================================


Vec2f XFont::GetTextSizeWInternal(const wchar_t* pBegin, const wchar_t* pEnd, const XTextDrawConect& ctx)
{
	X_PROFILE_BEGIN("FontTextSize", core::ProfileSubSys::FONT);

	float32_t maxW = 0;
	float32_t maxH = 0;

	Vec2f scale;
	float rcpCellWidth;

	// Scale it?
	Vec2f size = ctx.size; // in pixel
	if (ctx.flags.IsSet(DrawTextFlag::SCALE_800x600))
	{
		X_ASSERT_NOT_IMPLEMENTED();
	}

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
	const wchar_t* pCur = pBegin;
	while (pCur < pEnd)
	{
		const wchar_t ch = *pCur++;
		switch (ch)
		{
			case L'\\':
			{
				if (*pCur != L'n') {
					break;
				}

				++pCur;
			}
			case L'\n':
			{
				if (charX > maxW) {
					maxW = charX;
				}

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
				if (*pCur == L'^') {
					++pCur;
				}
				else if (*pCur)
				{
					++pCur;
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
			const int32_t charWidth = pFontTexture_->GetCharacterWidth(ch);
			advance = (charWidth + FONT_GLYPH_PROP_SPACING) * scale.x;
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

void XFont::Prepare(const wchar_t* pBegin, const wchar_t* pEnd)
{
	X_PROFILE_BEGIN("FontPrepare", core::ProfileSubSys::FONT);

	if (pFontTexture_->PreCacheString(pBegin, pEnd) == CacheResult::UPDATED)
	{
		fontTexDirty_ = true;
	}
}

bool XFont::CreateDeviceTexture(void)
{
	core::StackString512 name("$fontTexture_");
	name.append(getName().begin(), getName().end());

	const auto& buf = pFontTexture_->GetBuffer();

	X_ASSERT(pTexture_ == nullptr, "double init of font texture")(pTexture_);

	pTexture_ = gEnv->pRender->createTexture(
		name.c_str(),
		pFontTexture_->GetSize(),
		texture::Texturefmt::A8,
		buf.data()
	);

	if (!pTexture_->isLoaded()) {
		X_WARNING("Font", "Failed to create font texture.");
		return false;
	}

	// create a material here.
	// default_font
	auto* pMaterialMan = gEnv->p3DEngine->getMaterialManager();


	pMaterial_ = pMaterialMan->loadMaterial("code/default_font");
	// we wil lget back default if fails to load.
	// when we are default we ignore the textureSet and it just results in default texture been drawn.

	auto* pTech = pMaterialMan->getTechForMaterial(pMaterial_, core::StrHash("unlit"), render::shader::VertexFormat::P3F_T2S_C4B);
	if (!pTech) {
		X_ERROR("Font", "Failed to get 'unlit' tech for font");
		return false;
	}

	// set the texture id for the bound resource.
	if (!pMaterialMan->setTextureID(pMaterial_, pTech, core::StrHash("codeTexture0"), pTexture_->getTexID())) {
		X_ERROR("Font", "Failed to bind fontcache taxture to font state");
		return false;
	}

	return true;
}


size_t XFont::ByteToWide(const char* pBegin, const char* pEnd, wchar_t* pOut, size_t bufLen)
{
	size_t len = union_cast<size_t>(pEnd - pBegin);
	if (len > bufLen - 1) {
		X_ERROR("Font", "Wide conversion buffer overflow, clipping buffer. in: %" PRIuS " out: %" PRIuS, len, bufLen);
		len = bufLen - 1;
	}

	while (pBegin < pEnd)
	{
		*pOut++ = static_cast<uint8_t>(*pBegin++);
	}

	pOut[len] = 0;
	return len;
}

X_NAMESPACE_END
