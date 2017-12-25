#include "stdafx.h"
#include "Font.h"

#include "FontRender\XFontTexture.h"
#include "Sys\XFontSystem.h"

#include <IRender.h>
#include <I3DEngine.h>
#include <ICore.h>
#include <ITexture.h>
#include <IMaterial.h>
#include <IFileSys.h>
#include <IPrimativeContext.h>
#include <IRenderCommands.h>
#include <IFrameData.h>

#include <Math\VertexFormats.h>

#include <../../tools/MaterialLib/MatLib.h>

#include <CmdBucket.h>

X_NAMESPACE_BEGIN(font)

namespace
{

	Color8u g_ColorTable[10] = {
		Color8u(0, 0, 0),			// ^0 black
		Color8u(255, 0, 0),			// ^1 red
		Color8u(255, 240, 70),		// ^2 yellow
		Color8u(0, 0, 255),			// ^3 blue
		Color8u(0, 125, 255),		// ^4 light blue
		Color8u(255, 170, 155),		// ^5 light pink
		Color8u(255, 145, 50),		// ^6 orange
		Color8u(255, 255, 255),		// ^7 white
		Color8u(0, 255, 0),			// ^8 green
		Color8u(255, 105, 200)		// ^9 pink
	};
} // namespace


XFont::XFont(XFontSystem& fontSys, const char* pFontName) :
	fontSys_(fontSys),
	name_(pFontName),
	effects_(g_fontArena),
	pFontTexture_(nullptr),
	pTexture_(nullptr),
	fontTexDirty_(false),
	pMaterial_(nullptr),
	signal_(false),
	loadStatus_(core::LoadStatus::NotLoaded)
{
	X_ASSERT_NOT_NULL(g_fontArena);
}


XFont::~XFont()
{
	Free();

}

void XFont::Release()
{
	// what is this here for?
	X_ASSERT_NOT_IMPLEMENTED();
}


void XFont::Free(void)
{
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
		while (loadStatus_ == core::LoadStatus::Loading)
		{
			// if we have job system try help with work.
			// if no work wait...
			if (!gEnv->pJobSys || !gEnv->pJobSys->HelpWithWork())
			{
				signal_.wait();
			}
		}

		signal_.clear();

		if (loadStatus_ == core::LoadStatus::Error || loadStatus_ == core::LoadStatus::NotLoaded) {
			return false;
		}

		X_ASSERT_NOT_NULL(pFontTexture_);
	}

	return pFontTexture_->WaitTillReady();
}

void XFont::DrawTestText(engine::IPrimativeContext* pPrimCon, const core::FrameTimeData& time)
{
	font::TextDrawContext ctx;
	ctx.pFont = this;
	ctx.effectId = 0;
	ctx.SetSize(Vec2f(20.f, 20.f));
	ctx.flags = font::DrawTextFlag::FRAMED;

	// going to draw various test text.
	float posX = 30.f;
	float posY = 30.f;
	float SpacingX = 80.f;
	float SpacingY = 80.f;

	ctx.SetColor(Col_Aqua);
	ctx.SetCharWidthScale(1.0f);
	pPrimCon->drawText(10, posY, ctx, "Test text 20x20 1.0\tscale\nnew line 1\nnew line 2. :)");

	ctx.SetColor(Col_Khaki);
	ctx.SetCharWidthScale(0.5f);
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Test text 20x20 0.5\tscale\nnew line 1\nnew line 2 :|");

	ctx.SetColor(Col_Coral);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(20.f, 10.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Test text 20x10 1.0\tscale\nnew line 1\nnew line 2. :[");

	ctx.SetColor(Col_Mediumpurple);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(10.f, 20.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Test text 10x20 1.0\tscale\nnew line 1\nnew line 2. :[");

	ctx.SetColor(Col_Lime);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(10.f, 10.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Test text 10x10 1.0\tscale\nnew line 1\nnew line 2. :[");

	SpacingY = 90;

	ctx.SetColor(Col_Orangered);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(40.f, 40.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Test text 40x40 1.0\tscale");

	ctx.SetColor(Col_Orangered);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(64.f, 64.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Tgjycu {,}()£$% 64x64 1.0\tscale");

	ctx.SetColor(Col_Limegreen);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(48.f, 48.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Tgjycu {,}()£$% 48x48 1.0\tscale");

	ctx.SetColor(Col_Seashell);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(48.f, 48.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Tgjycu{,}()£$%48x481.0scale");

	ctx.SetColor(Col_Seashell);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(48.f, 48.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "s\tc\ta\tl\te|ygf|fvsv");


	ctx.SetColor(Col_Greenyellow);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(32.f, 32.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Tgjycu {,}()£$% 32x32 1.0\tscale");

	ctx.SetColor(Col_Steelblue);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(16.f, 16.f));
	pPrimCon->drawText(10, posY += SpacingY, ctx, "Tgjycu {,}()£$% 16x16 1.0\tscale");

	posX = 400.f;
	posY = 80.f;

	ctx.flags.Set(DrawTextFlag::CENTER);

	ctx.SetColor(Col_Steelblue);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(16.f, 16.f));
	pPrimCon->drawText(posX, posY, ctx, "lots\nof\nnew\nlines\nmeow\nmeow\nmeow\nyy||.");

	ctx.flags.Set(DrawTextFlag::CENTER_VER);

	ctx.SetColor(Col_Steelblue);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(16.f, 16.f));
	pPrimCon->drawText(posX += SpacingX, posY, ctx, "lots\nof\nnew\nlines\nmeow\nmeow\nmeow\nyy||.");

	ctx.flags.Remove(DrawTextFlag::CENTER);
	ctx.flags.Remove(DrawTextFlag::CENTER_VER);
	ctx.flags.Set(DrawTextFlag::RIGHT);

	ctx.SetColor(Col_Steelblue);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(16.f, 16.f));
	pPrimCon->drawText(posX += SpacingX, posY, ctx, "lots\nof\nnew\nlines\nmeow\nmeow\nmeow\nyy||.");

	ctx.flags.Remove(DrawTextFlag::CENTER);
	ctx.flags.Set(DrawTextFlag::CENTER_VER);
	ctx.flags.Set(DrawTextFlag::RIGHT);

	ctx.SetColor(Col_Steelblue);
	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(16.f, 16.f));
	pPrimCon->drawText(posX += SpacingX, posY, ctx, "lots\nof\nnew\nlines\nmeow\nmeow\nmeow\nyy||.");

	// draw load of centered single line vertically.
	ctx.flags.Remove(DrawTextFlag::CENTER_VER);
	ctx.flags.Remove(DrawTextFlag::RIGHT);
	ctx.flags.Set(DrawTextFlag::CENTER);

	SpacingY = 25.f;
	posX = 450.f;
	posY += 150.f;

	ctx.SetCharWidthScale(1.0f);
	ctx.SetSize(Vec2f(16.f, 16.f));
	ctx.SetColor(Col_Tomato);
	pPrimCon->drawText(posX, posY += SpacingY, ctx, "meow meow meow meow meow meow");
	ctx.SetColor(Col_Violet);
	pPrimCon->drawText(posX, posY += SpacingY, ctx, "meow meow meow meow meow");
	ctx.SetColor(Col_Plum);
	pPrimCon->drawText(posX, posY += SpacingY, ctx, "meow meow meow meow");
	ctx.SetColor(Col_Lightgoldenrodyellow);
	pPrimCon->drawText(posX, posY += SpacingY, ctx, "meow meow meow");
	ctx.SetColor(Col_Darksalmon);
	pPrimCon->drawText(posX, posY += SpacingY, ctx, "meow meow");

	// do some clip test.
	posX = 600.f;
	posY = 170;

	ctx.flags &= ~(DrawTextFlag::CENTER_VER | DrawTextFlag::CENTER | DrawTextFlag::RIGHT);
	ctx.flags.Set(DrawTextFlag::CLIP);
	
	// fixed clip test.

	ctx.clip.x1 = posX + 13.f;
	ctx.clip.y1 = posY + 12.f;
	ctx.clip.x2 = posX + 150.f ;
	ctx.clip.y2 = posY + 45.f;

	pPrimCon->drawText(posX, posY, ctx, "meow meow\tmeow meow\nmeow meow\t  OOOOO\nmeow\t\t\tmeow meow.");

	posY += 60.f;

	// move the clip around
	const float rangeX = 50.f;
	const float rangeY = 10.f;
	const float ellapsed = time.ellapsed[core::Timer::UI].GetMilliSeconds();

	float resultX = sin(ellapsed * 0.001f) * rangeX;
	float resultY = sin(ellapsed * 0.0001f) * rangeY;

	ctx.clip.x1 = posX + 13.f;
	ctx.clip.y1 = posY + 12.f;
	ctx.clip.x2 = posX + 100.f + resultX; 
	ctx.clip.y2 = posY + 35.f + resultY;

	pPrimCon->drawText(posX, posY, ctx, "meow meow\tmeow meow\nmeow meow\t  OOOOO\nmeow\t\t\tmeow meow.");

	posY += 60.f;

	ctx.clip.x1 = posX + 13.f + resultX;
	ctx.clip.y1 = posY + 12.f + resultY;
	ctx.clip.x2 = posX + 150.f;
	ctx.clip.y2 = posY + 45.f;

	pPrimCon->drawText(posX, posY, ctx, "meow meow\tmeow meow\nmeow meow\t  OOOOO\nmeow\t\t\tmeow meow.");

	// same agint not clipped
	ctx.flags.Remove(DrawTextFlag::CLIP);
	pPrimCon->drawText(posX, posY + 70.f, ctx, "meow meow\tmeow meow\nmeow meow\t  OOOOO\nmeow\t\t\tmeow");
}


void XFont::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
	const TextDrawContext& ctx, const char* pBegin, const char* pEnd)
{
	wchar_t strW[MAX_TXT_SIZE];

	size_t txtLen = pEnd - pBegin;
	const wchar_t* pWideBegin = core::strUtil::Convert(pBegin, strW);
	const wchar_t* pWideEnd = pWideBegin + txtLen;


	DrawStringInternal(
		pPrimCon,
		pos,
		ctx,
		pWideBegin,
		pWideEnd,
		&ang
	);
}

void XFont::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos, const Matrix33f& ang,
	const TextDrawContext& ctx, const wchar_t* pBegin, const wchar_t* pEnd)
{
	DrawStringInternal(
		pPrimCon,
		pos,
		ctx,
		pBegin,
		pEnd,
		&ang
	);
}

void XFont::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
	const TextDrawContext& ctx, const char* pBegin, const char* pEnd)
{
	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	
	size_t txtLen = pEnd - pBegin;
	const wchar_t* pWideBegin = core::strUtil::Convert(pBegin, strW);
	const wchar_t* pWideEnd = pWideBegin + txtLen;

	DrawStringInternal(
		pPrimCon,
		pos,
		ctx,
		pWideBegin,
		pWideEnd,
		nullptr
	);
}

void XFont::DrawString(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
	const TextDrawContext& ctx, const wchar_t* pBegin, const wchar_t* pEnd)
{
	DrawStringInternal(
		pPrimCon,
		pos,
		ctx,
		pBegin,
		pEnd,
		nullptr
	);
}

void XFont::DrawStringInternal(engine::IPrimativeContext* pPrimCon, const Vec3f& pos,
	const TextDrawContext& ctx, const wchar_t* pBegin, const wchar_t* pEnd, const Matrix33f* pRotation)
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
		X_WARNING("Font", "\"%s\" has no effects.", getName().c_str());
		return;
	}

	X_ASSERT(ctx.GetEffectId() >= 0 && ctx.GetEffectId() < static_cast<int32_t>(effects_.size()), 
		"Effect index invalid")(ctx.GetEffectId(), effects_.size());

	// updates LRU cache and adds glyphs to font texture if missing.
	// the device texture is not updated here tho.
	Prepare(pBegin, pEnd);


	const bool isProportional = flags_.IsSet(FontFlag::PROPORTIONAL);
	const bool drawFrame = ctx.flags.IsSet(DrawTextFlag::FRAMED);
	const bool clipped = ctx.flags.IsSet(DrawTextFlag::CLIP);
	const bool shiftedPosition = (ctx.flags & (DrawTextFlag::CENTER | DrawTextFlag::CENTER_VER | DrawTextFlag::RIGHT)).IsAnySet();
	const bool debugGlyphRect = fontSys_.getVars().glyphDebugRect();
	const bool debugRect = fontSys_.getVars().debugRect();
	const bool debugPos = fontSys_.getVars().debugShowDrawPosition();
	const auto effecIdx = ctx.GetEffectId();


	const Metrics& metrics = pFontTexture_->GetMetrics();
	const float scaleX = ctx.size.x / pFontTexture_->GetCellWidth();
	const float scaleY = ctx.size.y / pFontTexture_->GetCellHeight();
	const float verBase = ctx.size.y - (metrics.ascender * scaleY); // we need to take 64 - this scaled.
	const float hozAdvance = (metrics.max_advance * scaleX);
	
	// for render don't think i want to move down including descenders only for text size.
	// const float verAdvance = ctx.size.y + -(metrics.descender * scaleY);
	const float verAdvance = ctx.size.y;


	Vec2f textSize = Vec2f::zero();	
	Vec2f baseXY(pos.x, pos.y);

	if (debugPos)
	{
		pPrimCon->drawCrosshair(pos, 6.f, Col_Red);
	}

	if (shiftedPosition)
	{
		textSize = GetTextSizeWInternal(pBegin, pEnd, ctx);

		if (ctx.flags.IsSet(DrawTextFlag::RIGHT))
		{
			baseXY.x -= textSize.x;
		}
		else if (ctx.flags.IsSet(DrawTextFlag::CENTER))
		{
			baseXY.x -= (textSize.x / 2.f);
		}

 		if (ctx.flags.IsSet(DrawTextFlag::CENTER_VER))
		{
			baseXY.y -= (textSize.y / 2.f);
		}
	}

	// snap.
	if (pos.z == 0.f) // makes 3d text jump. (FIXME: this is not a proper fix really need to know if 3d or not)
	{
		baseXY.x = math<float>::floor(baseXY.x);
		baseXY.y = math<float>::floor(baseXY.y);
	}

	FontEffect& effect = effects_[effecIdx];
	for (auto passIdx = 0u; passIdx < effect.passes.size(); passIdx++)
	{
		const FontPass& pass = effect.passes[passIdx];

		Color8u col = ctx.col;

		float z = pos.z;
		float charX = baseXY.x + pass.offset.x;
		float charY = baseXY.y + pass.offset.y;


		if (drawFrame && passIdx == 0)
		{
			if (!shiftedPosition) {
				textSize = GetTextSizeWInternal(pBegin, pEnd, ctx);
			}

			Vec2f baseOffset;
			if (ctx.flags.IsSet(DrawTextFlag::FRAMED_SNUG)) {
				baseOffset.y = verBase;
			}
			else {
			//	this won't handle multiline correct.
			//	textSize.y = verAdvance;
			}

			const Color8u frameColor(7, 7, 7, 80); //dark grey, 65% opacity

			float x0 = baseXY.x + baseOffset.x;
			float y0 = baseXY.y + baseOffset.y;
			float x1 = baseXY.x + textSize.x;
			float y1 = baseXY.y + textSize.y;

			// are we clipping or are we whipping!
			if (clipped)
			{
				auto& clip = ctx.clip;

				x0 = core::Max(x0, clip.x1);
				y0 = core::Max(y0, clip.y1);
				x1 = core::Min(x1, clip.x2);
				y1 = core::Min(y1, clip.y2);
			}

			Vec3f v0(x0, y0, z);
			Vec3f v2(x1, y1, z);
			Vec3f v1(v2.x, v0.y, v0.z); // to avoid float->half conversion
			Vec3f v3(v0.x, v2.y, v0.z); // to avoid float->half conversion

			if (pRotation)
			{
				auto& rot = *pRotation;

				Vec3f base(pos);
				v0 = (rot * (v0 - base)) + base;
				v1 = (rot * (v1 - base)) + base;
				v2 = (rot * (v2 - base)) + base;
				v3 = (rot * (v3 - base)) + base;
			}


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
				case L'\t':
				case L' ':
				{
					const int32_t numSpaces = ch == L' ' ? 1 : FONT_TAB_CHAR_NUM;

					if (isProportional)
					{
						auto* pSlot = pFontTexture_->GetCharSlot(' ');
						if (pSlot)
						{
							charX += numSpaces * (pSlot->advanceX * scaleX);
						}
					}
					else
					{
						charX += (hozAdvance * numSpaces);
					}
					continue;
				}

				case L'^':
				{
					if (*pChar == L'^') {
						++pChar;
					}
					else if (*pChar == L'~') { // resets to contex col
						col = ctx.col; 
						++pChar;
					}
					else if (core::strUtil::IsDigitW(*pChar))
					{
						const int32_t colorIndex = (*pChar) - L'0';
						X_ASSERT(colorIndex >= 0 && colorIndex < X_ARRAY_SIZE(g_ColorTable), "ColorIndex out of range")(colorIndex);
						Color8u newColor = g_ColorTable[colorIndex];
						col.r = newColor.r;
						col.g = newColor.g;
						col.b = newColor.b;
						++pChar;
					}
					continue;
				}

				case L'\n':
				{
					charX = baseXY.x;
					charY += verAdvance;
					continue;
				}

				case L'\r':
				{
					charX = baseXY.x;
					continue;
				}

				default:
					break;
			}

			const XTextureSlot* pSlot = pFontTexture_->GetCharSlot(ch);
			XCharCords cords;
			pFontTexture_->GetTextureCoord(pSlot, cords);

			float xpos = charX + cords.offset.x * scaleX;
			float ypos = charY + cords.offset.y * scaleY;
			float right = xpos + cords.size.x * scaleX;
			float bottom = ypos + cords.size.y * scaleY;


			// Verts
			Vec3f tl(xpos, ypos, z);
			Vec3f br(right, bottom, z);
			Vec3f tr(br.x, tl.y, z);
			Vec3f bl(tl.x, br.y, z);

			if (pRotation)
			{
				auto& rot = *pRotation;

				Vec3f base(pos);
				tl = (rot * (tl - base)) + base;
				br = (rot * (br - base)) + base;
				tr = (rot * (tr - base)) + base;
				bl = (rot * (bl - base)) + base;
			}


			float hozAdvanceChar = hozAdvance;
			if (isProportional) {
				hozAdvanceChar = pSlot->advanceX * scaleX;
			}

			if (clipped)
			{
				auto& clip = ctx.clip;

				Rectf charRect(tl.x, tl.y, br.x, br.y);

				// even in the rect?
				if (!clip.intersects(charRect))
				{
					charX += hozAdvanceChar;
					continue;
				}

				// are we partial?
				if (!clip.contains(charRect))
				{
					// so we want to clip the verts down and adjust the uv's
					// should just process each point.

					// left
					if (tl.x < clip.x1)
					{
						const float diff = clip.x1 - tl.x;
						const float width = tr.x - tl.x;

						tl.x = clip.x1;
						bl.x = clip.x1;

						// we need to find out how much we moved and lerp towards to other uv.
						float factor = diff / width;
						cords.texCoords[0] = lerp(cords.texCoords[0], cords.texCoords[2], factor);
					}

					// top
					if (tl.y < clip.y1)
					{
						const float diff = clip.y1 - tl.y;
						const float height = bl.y - tl.y;

						tl.y = clip.y1;
						tr.y = clip.y1;

						float factor = diff / height;
						cords.texCoords[1] = lerp(cords.texCoords[1], cords.texCoords[3], factor);
					}

					// right
					if (tr.x > clip.x2)
					{
						const float diff = tr.x - clip.x2;
						const float width = tr.x - tl.x;

						tr.x = clip.x2;
						br.x = clip.x2;

						float factor = diff / width;
						cords.texCoords[2] = lerp(cords.texCoords[2], cords.texCoords[0], factor);
					}

					// bottom
					if (bl.y > clip.y2)
					{
						const float diff =  bl.y - clip.y2;
						const float height = bl.y - tl.y;

						bl.y = clip.y2;
						br.y = clip.y2;

						float factor = diff / height;
						cords.texCoords[3] = lerp(cords.texCoords[3], cords.texCoords[1], factor);
					}
				}
			}

			// final cords
			const core::XHalf2 tc_tl(cords.texCoords[0], cords.texCoords[1]);
			const core::XHalf2 tc_br(cords.texCoords[2], cords.texCoords[3]);
			const core::XHalf2 tc_tr(tc_br.x, tc_tl.y);
			const core::XHalf2 tc_bl(tc_tl.x, tc_br.y);


			const Color8u finalCol = pass.col * col;

			// We need 6 since each char is not connected.
			// so we make seprate tri's
			engine::IPrimativeContext::PrimVertex* pVerts = pPrimCon->addPrimative(6, render::TopoType::TRIANGLELIST, pMaterial_);

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

			charX += hozAdvanceChar;
		}
	}

	// we draw the rects after all chars are drawn, otherwise performance will tank.
	// as all the batching goes to shit.
	if (debugRect)
	{
		if (!shiftedPosition) {
			textSize = GetTextSizeWInternal(pBegin, pEnd, ctx);
		}

		Vec3f tl(baseXY.x, baseXY.y, pos.z);
		Vec3f tr(tl.x + textSize.x, tl.y, pos.z);
		Vec3f bl(tl.x, tl.y + textSize.y, pos.z);
		Vec3f br(tr.x, bl.y, pos.z);

		if (pRotation)
		{
			auto& rot = *pRotation;

			Vec3f base(pos);
			tl = (rot * (tl - base)) + base;
			br = (rot * (br - base)) + base;
			tr = (rot * (tr - base)) + base;
			bl = (rot * (bl - base)) + base;
		}

		pPrimCon->drawRect(tl, tr, bl, br, Col_Red);
	}

	if (debugGlyphRect)
	{
		const FontPass& pass = effect.passes[0];

		float charX = baseXY.x + pass.offset.x;
		float charY = baseXY.y + pass.offset.y;

		const wchar_t* pChar = pBegin;
		while (wchar_t ch = *pChar++)
		{
			// check for special chars that alter how we render.
			switch (ch)
			{
				case L'\t':
				case L' ':
				{
					const int32_t numSpaces = ch == L' ' ? 1 : FONT_TAB_CHAR_NUM;

					if (isProportional)
					{
						auto* pSlot = pFontTexture_->GetCharSlot(' ');
						if (pSlot)
						{
							charX += numSpaces * (pSlot->advanceX * scaleX);
						}
					}
					else
					{
						charX += (hozAdvance * numSpaces);
					}
					continue;
				}

				case L'^':
				{
					if (*pChar == L'^') {
						++pChar;
					}
					else if (core::strUtil::IsDigitW(*pChar))
					{
						++pChar;
					}
					continue;
				}

				case L'\n':
				{
					charX = baseXY.x;
					charY += verAdvance;
					continue;
				}

				case L'\r':
				{
					charX = baseXY.x;
					continue;
				}

				default:
					break;
			}

			float hozAdvanceChar = hozAdvance;
			if (isProportional) 
			{
				const XTextureSlot* pSlot = pFontTexture_->GetCharSlot(ch);
				hozAdvanceChar = pSlot->advanceX * scaleX;
			}

			Vec3f tl(charX, charY, pos.z);
			Vec3f tr(tl.x + hozAdvanceChar, tl.y, pos.z);
			Vec3f bl(tl.x, tl.y + ctx.size.y, pos.z);
			Vec3f br(tr.x, bl.y, pos.z);

			if (pRotation)
			{
				auto& rot = *pRotation;

				tl = (rot * (tl - pos)) + pos;
				br = (rot * (br - pos)) + pos;
				tr = (rot * (tr - pos)) + pos;
				bl = (rot * (bl - pos)) + pos;
			}

			pPrimCon->drawRect(tl, tr, bl, br, Col_Red);

			charX += hozAdvanceChar;
		}
	}
}


Vec2f XFont::GetTextSizeWInternal(const wchar_t* pBegin, const wchar_t* pEnd, const TextDrawContext& ctx)
{
	X_PROFILE_BEGIN("FontTextSize", core::profiler::SubSys::FONT);

	const bool isProportional = flags_.IsSet(FontFlag::PROPORTIONAL);
	const Metrics& metrics = pFontTexture_->GetMetrics();

	const float scaleX = ctx.size.x / pFontTexture_->GetCellWidth();
	const float scaleY = ctx.size.y / pFontTexture_->GetCellHeight();
//	const float verBase = ctx.size.y - (metrics.ascender * scaleY); // we need to take 64 - this scaled.
	const float hozAdvance =  (metrics.max_advance * scaleX);
	const float verAdvance = ctx.size.y + -(metrics.descender * scaleY);

	// starting points.
	float charX = 0;
	float charY = 0 + verAdvance;
	// we reset width, so need to keep track of max.
	float maxW = 0;

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
				maxW = core::Max(maxW, charX);

				charX = 0;
				// really we need to advance by min of size.
				charY += core::Min(verAdvance, ctx.size.y);
				continue;
			}
			case L'\r':
			{
				maxW = core::Max(maxW, charX);

				charX = 0;
				continue;
			}
			case L'\t':
			case L' ':
			{
				const int32_t numSpaces = ch == L' ' ? 1 : FONT_TAB_CHAR_NUM;

				if (isProportional)
				{
					auto* pSlot = pFontTexture_->GetCharSlot(' ');
					if (pSlot)
					{
						charX += numSpaces * (pSlot->advanceX * scaleX);
					}
				}
				else
				{
					charX += (hozAdvance * numSpaces);
				}
				continue;
			}
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

		if (isProportional)
		{
			auto* pSlot = pFontTexture_->GetCharSlot(ch);
			if (pSlot)
			{
				charX += pSlot->advanceX * scaleX;
			}
		}
		else
		{
			charX += hozAdvance;
		}
	}

	maxW = core::Max(maxW, charX);

	return Vec2f(maxW, charY);
}


// ---------------------------------------------------------

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

// ---------------------------------------------------------


// calculate the size.
Vec2f XFont::GetTextSize(const char* pBegin, const char* pEnd, const TextDrawContext& contex)
{
	// to wide char
	wchar_t strW[MAX_TXT_SIZE];
	const size_t len = ByteToWide(pBegin, pEnd, strW, MAX_TXT_SIZE);

	return GetTextSizeWInternal(strW, strW + len, contex);
}

Vec2f XFont::GetTextSize(const wchar_t* pBegin, const wchar_t* pEnd, const TextDrawContext& contex)
{
	return GetTextSizeWInternal(pBegin, pEnd, contex);
}

float32_t XFont::GetCharWidth(wchar_t cChar, size_t num, const TextDrawContext& contex)
{
	Vec2f size = GetTextSizeWInternal(&cChar, &cChar + 1, contex);

	return size.x * num;
}

// ---------------------------------------------------------


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

// ---------------------------------------------------------


void XFont::Prepare(const wchar_t* pBegin, const wchar_t* pEnd)
{
	X_PROFILE_BEGIN("FontPrepare", core::profiler::SubSys::FONT);

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
		render::BufUsage::DYNAMIC,
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

// ---------------------------------------------------------

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
