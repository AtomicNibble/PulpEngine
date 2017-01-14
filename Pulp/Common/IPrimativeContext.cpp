#include "EngineCommon.h"
#include "IPrimativeContext.h"

#include "Math\VertexFormats.h"


X_NAMESPACE_BEGIN(engine)


IPrimativeContext::IPrimativeContext()
{

}

IPrimativeContext::~IPrimativeContext()
{

}

void IPrimativeContext::drawQuadSS(float x, float y, float width, float height, const Color& col)
{
	// input is 0-2
	// directx is like so:
	//
	// -1,1			1,1
	//
	//		  0,0
	//
	// -1,-1		1,-1
	float x1, y1, x2, y2;
	float z;

	z = 0.f;
	x1 = x - 1;
	y1 = 2.f - y;

	// now we need to add width / hiehgt
	x2 = x1 + width;
	y2 = y1 - height;


	PrimVertex* pQuad = addPrimative(4, PrimitiveType::TRIANGLESTRIP);

	// TL
	pQuad[0].pos.x = x1;
	pQuad[0].pos.y = y1;
	pQuad[0].pos.z = z;
	// TR
	pQuad[1].pos.x = x2;
	pQuad[1].pos.y = y1;
	pQuad[1].pos.z = z;
	// BL
	pQuad[2].pos.x = x1;
	pQuad[2].pos.y = y2;
	pQuad[2].pos.z = z;
	// BR
	pQuad[3].pos.x = x2;
	pQuad[3].pos.y = y2;
	pQuad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		pQuad[i].color = col;
		pQuad[i].st = core::XHalf2::zero();
	}
}

void IPrimativeContext::drawQuadSS(const Rectf& rect, const Color& col)
{
	// input is 0-2
	// directx is like so:
	//
	// -1,1			1,1
	//
	//		  0,0
	//
	// -1,-1		1,-1
	float x1, y1, x2, y2;
	float z;

	z = 0.f;
	x1 = rect.x1 - 1.f;
	y1 = 1.f - rect.y1;
	x2 = rect.x2 - 1.f;
	y2 = 1.f - rect.y2;

	PrimVertex* pQuad = addPrimative(4, PrimitiveType::TRIANGLESTRIP);

	// TL
	pQuad[0].pos.x = x1;
	pQuad[0].pos.y = y1;
	pQuad[0].pos.z = z;
	// TR
	pQuad[1].pos.x = x2;
	pQuad[1].pos.y = y1;
	pQuad[1].pos.z = z;
	// BL
	pQuad[2].pos.x = x1;
	pQuad[2].pos.y = y2;
	pQuad[2].pos.z = z;
	// BR
	pQuad[3].pos.x = x2;
	pQuad[3].pos.y = y2;
	pQuad[3].pos.z = z;


	for (uint32 i = 0; i<4; ++i)
	{
		pQuad[i].color = col;
		pQuad[i].st = core::XHalf2::zero();
	}
}


void IPrimativeContext::drawQuadImageSS(const Rectf& rect, Material* pMaterial, const Color& col)
{
	float x1, y1, x2, y2;
	float z;
	float s[4], t[4];

	s[0] = 0;	t[0] = 1.0f - 1;
	s[1] = 1;	t[1] = 1.0f - 1;
	s[2] = 0;	t[2] = 1.0f - 0;
	s[3] = 1;	t[3] = 1.0f - 0;

	z = 0.f;
	x1 = rect.x1 - 1;
	y1 = 2.f - rect.y1;
	x2 = x1 + rect.getWidth();
	y2 = y1 - rect.getHeight();

	PrimVertex* pQuad = addPrimative(4, PrimitiveType::TRIANGLESTRIP, pMaterial);

	// TL
	pQuad[0].pos.x = x1;
	pQuad[0].pos.y = y1;
	pQuad[0].pos.z = z;
	// TR
	pQuad[1].pos.x = x2;
	pQuad[1].pos.y = y1;
	pQuad[1].pos.z = z;
	// BL
	pQuad[2].pos.x = x1;
	pQuad[2].pos.y = y2;
	pQuad[2].pos.z = z;
	// BR
	pQuad[3].pos.x = x2;
	pQuad[3].pos.y = y2;
	pQuad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		pQuad[i].color = col;
		pQuad[i].st = core::XHalf2::compress(s[i], t[i]);
	}
}

void IPrimativeContext::drawRectSS(const Rectf& rect, const Color& col)
{
	float x1, y1, x2, y2;

	x1 = rect.x1 - 1.f;
	y1 = 1.f - rect.y1;
	x2 = rect.x2 - 1.f;
	y2 = 1.f - rect.y2;

	const Vec3f tl(x1, y1, 0);
	const Vec3f tr(x2, y1, 0);
	const Vec3f bl(x1, y2, 0);
	const Vec3f br(x2, y2, 0);

	// once the coords are mapped to directx coords.
	// we can just use the default none SS functions.
	// since the shader is what's important.

	// Top
	drawLine(tl, col, tr, col);
	// bottom
	drawLine(bl, col, br, col);
	// left down
	drawLine(tl, col, bl, col);
	// right down
	drawLine(tr, col, br, col);
}

void IPrimativeContext::drawLineSS(const Vec2f& vPos1, const Color& color1,
	const Vec2f& vPos2, const Color& color2)
{
	Vec3f pos1, pos2;

	pos1.x = vPos1.x - 1;
	pos1.y = 1.f - vPos1.y;

	pos2.x = vPos2.x - 1;
	pos2.y = 1.f - vPos2.y;

	drawLine(pos1, color1, pos2, color2);
}

void IPrimativeContext::drawQuad(float x, float y, float z, float width, float height, const Color& col)
{
	const float fx = x;
	const float fy = y;
	const float fz = z;
	const float fw = width;
	const float fh = height;

	PrimVertex* pQuad = addPrimative(4, PrimitiveType::TRIANGLESTRIP);

	// TL
	pQuad[0].pos.x = fx;
	pQuad[0].pos.y = fy;
	pQuad[0].pos.z = fz;
	// TR
	pQuad[1].pos.x = fx + fw;
	pQuad[1].pos.y = fy;
	pQuad[1].pos.z = fz;
	// BL
	pQuad[2].pos.x = fx;
	pQuad[2].pos.y = fy + fh;
	pQuad[2].pos.z = fz;
	// BR
	pQuad[3].pos.x = fx + fw;
	pQuad[3].pos.y = fy + fh;
	pQuad[3].pos.z = fz;

	for (uint32 i = 0; i<4; ++i)
	{
		pQuad[i].color = col;
		pQuad[i].st = core::XHalf2::zero();
	}
}


void IPrimativeContext::drawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col)
{
	PrimVertex* pQuad = addPrimative(4, PrimitiveType::TRIANGLESTRIP);

	pQuad[0].pos = pos0;
	pQuad[1].pos = pos1;
	pQuad[2].pos = pos2;
	pQuad[3].pos = pos3;

	for (uint32 i = 0; i < 4; ++i)
	{
		pQuad[i].color = col;
		pQuad[i].st = core::XHalf2::zero();
	}
}

void IPrimativeContext::drawLines(Vec3f* pPoints, uint32_t num, const Color& col)
{
	X_ASSERT_NOT_NULL(pPoints);
	X_ASSERT((num % 2) == 0, "num points must be a multiple of 2")(num);

	if (num < 2) { // 2 points needed to make a line.
		return;
	}

	PrimVertex* pLine = addPrimative(num, PrimitiveType::LINELIST);

	for (uint32_t i = 0; i < num / 2; i++)
	{
		pLine[i].pos = pPoints[i];
		pLine[i].color = col;
		pLine[i].st = core::XHalf2::zero();
		pLine[i + 1].pos = pPoints[i + 1];
		pLine[i + 1].color = col;
		pLine[i + 1].st = core::XHalf2::zero();
	}
}


void IPrimativeContext::drawRect(float x, float y, float width, float height, const Color& col)
{
	const float x1 = x;
	const float y1 = y;
	const float x2 = x + width;
	const float y2 = y + height;

	const Vec3f tl(x1, y1, 0);
	const Vec3f tr(x2, y1, 0);
	const Vec3f bl(x1, y2, 0);
	const Vec3f br(x2, y2, 0);

	// Top
	drawLine(tl, col, tr, col);
	// bottom
	drawLine(bl, col, br, col);
	// left down
	drawLine(tl, col, bl, col);
	// right down
	drawLine(tr, col, br, col);
}

void IPrimativeContext::drawBarChart(const Rectf& rect, uint32_t num, float* pHeights, 
	float padding, uint32_t max, const Color& col)
{
	X_ASSERT_NOT_NULL(pHeights);
	X_ASSERT(num <= max, "Darw Chart has more items than max")(num, max);

	if (num < 1) {
		return;
	}

	// calculate the bar width.
	const float bar_width = ((rect.getWidth() / max) - padding) + padding / max;
	const float bottom = rect.getY2();
	const float height = rect.getHeight();
	float right = rect.getX2();

	const Color8u col8(col);

	PrimVertex* pQuads = addPrimative(num * 6, PrimitiveType::LINELIST);

	// TL - TR - BR
	// BR - BL - TL
	for (uint32_t i = 0; i < num; i++)
	{
		PrimVertex* pQuad = &pQuads[i * 6];
		float cur_bar = pHeights[i];

		// TL
		pQuad[0].pos.x = right - bar_width;
		pQuad[0].pos.y = bottom - (height * cur_bar);
		pQuad[0].pos.z = 0.f;
		pQuad[0].color = col8;

		// TR
		pQuad[1].pos.x = right;
		pQuad[1].pos.y = bottom - (height * cur_bar);
		pQuad[1].pos.z = 0.f;
		pQuad[1].color = col8;

		// BR
		pQuad[2].pos.x = right;
		pQuad[2].pos.y = bottom;
		pQuad[2].pos.z = 0.f;
		pQuad[2].color = col8;

		// BR
		pQuad[3].pos.x = right;
		pQuad[3].pos.y = bottom;
		pQuad[3].pos.z = 0.f;
		pQuad[3].color = col8;

		// BL
		pQuad[4].pos.x = right - bar_width;
		pQuad[4].pos.y = bottom;
		pQuad[4].pos.z = 0.f;
		pQuad[4].color = col8;

		// TL
		pQuad[5].pos.x = right - bar_width;
		pQuad[5].pos.y = bottom - (height * cur_bar);
		pQuad[5].pos.z = 0.f;
		pQuad[5].color = col8;

		right -= (bar_width + padding);
	}

}


void IPrimativeContext::drawTriangle(const Vec3f* pPoints, size_t numPoints, const Color8u& c0)
{
	if (numPoints == 0) {
		return;
	}

	X_ASSERT(numPoints % 3 == 0, "Num points must be a multiple of 3")(numPoints);

	// we do the case to uint32_t here just to make the interface nicer to use.
	PrimVertex* pTri = addPrimative(static_cast<uint32_t>(numPoints), PrimitiveType::TRIANGLELIST);

	for (size_t i = 0; i < numPoints; i++)
	{
		pTri[i].pos = pPoints[i];
		pTri[i].color = c0;
	}
}

void IPrimativeContext::drawTriangle(const Vec3f* pPoints, size_t numPoints, const Color8u* pCol)
{
	if (numPoints == 0) {
		return;
	}

	X_ASSERT(numPoints % 3 == 0, "Num points must be a multiple of 3")(numPoints);


	PrimVertex* pTri = addPrimative(static_cast<uint32_t>(numPoints), PrimitiveType::TRIANGLELIST);

	for (size_t i = 0; i < numPoints; i++)
	{
		pTri[i].pos = pPoints[i];
		pTri[i].color = pCol[i];
	}
}
void IPrimativeContext::drawImageWithUV(float xpos, float ypos, float z, float w, float h,
	Material* pMaterial, const float* s, const float* t,
	const Colorf& col, bool filtered)
{
	X_ASSERT_NOT_NULL(s);
	X_ASSERT_NOT_NULL(t);
	X_UNUSED(filtered);

	PrimVertex* pQuad = addPrimative(4, PrimitiveType::TRIANGLESTRIP, pMaterial);

	// TL
	pQuad[0].pos.x = xpos;
	pQuad[0].pos.y = ypos;
	pQuad[0].pos.z = z;
	// TR
	pQuad[1].pos.x = xpos + w;
	pQuad[1].pos.y = ypos;
	pQuad[1].pos.z = z;
	// BL
	pQuad[2].pos.x = xpos;
	pQuad[2].pos.y = ypos + h;
	pQuad[2].pos.z = z;
	// BR
	pQuad[3].pos.x = xpos + w;
	pQuad[3].pos.y = ypos + h;
	pQuad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		pQuad[i].color = col;
		pQuad[i].st = core::XHalf2::compress(s[i], t[i]);
	}
}


X_NAMESPACE_END