#include "stdafx.h"
#include "Dx10Render.h"
#include "../Common/Textures/XTexture.h"

X_NAMESPACE_BEGIN(render)



void DX11XRender::DrawQuadSS(float x, float y, float width, float height, const Color& col)
{
	SetCullMode(CullMode::NONE);

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


	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = x1;
	Quad[0].pos.y = y1;
	Quad[0].pos.z = z;
	// TR
	Quad[1].pos.x = x2;
	Quad[1].pos.y = y1;
	Quad[1].pos.z = z;
	// BL
	Quad[2].pos.x = x1;
	Quad[2].pos.y = y2;
	Quad[2].pos.z = z;
	// BR
	Quad[3].pos.x = x2;
	Quad[3].pos.y = y2;
	Quad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f::zero();
	}

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}

void DX11XRender::DrawQuadSS(const Rectf& rect, const Color& col)
{
	SetCullMode(CullMode::NONE);

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


	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = x1;
	Quad[0].pos.y = y1;
	Quad[0].pos.z = z;
	// TR
	Quad[1].pos.x = x2;
	Quad[1].pos.y = y1;
	Quad[1].pos.z = z;
	// BL
	Quad[2].pos.x = x1;
	Quad[2].pos.y = y2;
	Quad[2].pos.z = z;
	// BR
	Quad[3].pos.x = x2;
	Quad[3].pos.y = y2;
	Quad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f::zero();
	}

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}

void DX11XRender::DrawQuadSS(float x, float y, float width, float height,
	const Color& col, const Color& borderCol)
{
	DrawQuadSS(x, y, width, height, col);
	DrawRectSS(x, y, width, height, borderCol);
}

void DX11XRender::DrawQuadImageSS(float x, float y, float width, float height,
	texture::TexID texture_id, const Color& col)
{
	float x1, y1, x2, y2;
	float z;
	float s[4], t[4];

	s[0] = 0;	t[0] = 1.0f - 1;
	s[1] = 1;	t[1] = 1.0f - 1;
	s[2] = 0;	t[2] = 1.0f - 0;
	s[3] = 1;	t[3] = 1.0f - 0;

	z = 0.f;
	x1 = x - 1;
	y1 = 2.f - y;
	x2 = x1 + width;
	y2 = y1 - height;


	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = x1;
	Quad[0].pos.y = y1;
	Quad[0].pos.z = z;
	// TR
	Quad[1].pos.x = x2;
	Quad[1].pos.y = y1;
	Quad[1].pos.z = z;
	// BL
	Quad[2].pos.x = x1;
	Quad[2].pos.y = y2;
	Quad[2].pos.z = z;
	// BR
	Quad[3].pos.x = x2;
	Quad[3].pos.y = y2;
	Quad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f(s[i], t[i]);
	}

	// We are finished with accessing the vertex buffer
	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	// Bind our vertex as the first data stream of our device
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();


	texture::XTexture::applyFromId(0, texture_id, 0);

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}

void DX11XRender::DrawQuadImageSS(const Rectf& rect, texture::TexID texture_id, const Color& col)
{
	float x1, y1, x2, y2;
	float z;
	float s[4], t[4];

	s[0] = 0;	t[0] = 1.0f - 1;
	s[1] = 1;	t[1] = 1.0f - 1;
	s[2] = 0;	t[2] = 1.0f - 0;
	s[3] = 1;	t[3] = 1.0f - 0;

	z = 0.f;
	x1 = rect.x1 - 1.f;
	y1 = 1.f - rect.y1;
	x2 = rect.x2 - 1.f;
	y2 = 1.f - rect.y2;


	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = x1;
	Quad[0].pos.y = y1;
	Quad[0].pos.z = z;
	// TR
	Quad[1].pos.x = x2;
	Quad[1].pos.y = y1;
	Quad[1].pos.z = z;
	// BL
	Quad[2].pos.x = x1;
	Quad[2].pos.y = y2;
	Quad[2].pos.z = z;
	// BR
	Quad[3].pos.x = x2;
	Quad[3].pos.y = y2;
	Quad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f(s[i], t[i]);
	}

	// We are finished with accessing the vertex buffer
	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	// Bind our vertex as the first data stream of our device
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();


	texture::XTexture::applyFromId(0, texture_id, 0);

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}


void DX11XRender::DrawRectSS(float x, float y, float width, float height, const Color& col)
{
	float x1, y1, x2, y2;

	x1 = x - 1;
	y1 = 2.f - y;
	x2 = x1 + width;
	y2 = y1 - height;

	const Vec3f tl(x1, y1, 0);
	const Vec3f tr(x2, y1, 0);
	const Vec3f bl(x1, y2, 0);
	const Vec3f br(x2, y2, 0);

	// once the coords are mapped to directx coords.
	// we can just use the default none SS functions.
	// since the shader is what's important.

	// Top
	DrawLineColor(tl, col, tr, col);
	// bottom
	DrawLineColor(bl, col, br, col);
	// left down
	DrawLineColor(tl, col, bl, col);
	// right down
	DrawLineColor(tr, col, br, col);
}

void DX11XRender::DrawRectSS(const Rectf& rect, const Color& col)
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
	DrawLineColor(tl, col, tr, col);
	// bottom
	DrawLineColor(bl, col, br, col);
	// left down
	DrawLineColor(tl, col, bl, col);
	// right down
	DrawLineColor(tr, col, br, col);
}

void DX11XRender::DrawLineColorSS(const Vec2f& vPos1, const Color& color1,
	const Vec2f& vPos2, const Color& color2)
{
	Vec3f pos1, pos2;

	pos1.x = vPos1.x - 1;
	pos1.y = 1.f - vPos1.y;

	pos2.x = vPos2.x - 1;
	pos2.y = 1.f - vPos2.y;

	DrawLineColor(pos1, color1, pos2, color2);
}


void DX11XRender::DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2,
	const Vec3f& pos3, const Color& col)
{

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	Quad[0].pos = pos0;
	Quad[1].pos = pos1;
	Quad[2].pos = pos2;
	Quad[3].pos = pos3;


	for (uint32 i = 0; i < 4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f::zero();
	}

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}


void DX11XRender::DrawQuadImage(float xpos, float ypos,
	float w, float h, texture::TexID texture_id, const Color& col)
{
	DrawImage(xpos, ypos, 0.f, w, h, texture_id, 0, 1, 1, 0, col);
}

void DX11XRender::DrawQuadImage(float xpos, float ypos,
	float w, float h, texture::ITexture* pTexutre, const Color& col)
{
	DrawImage(xpos, ypos, 0.f, w, h, pTexutre->getTexID(), 0, 1, 1, 0, col);
}

void DX11XRender::DrawQuadImage(const Rectf& rect, texture::ITexture* pTexutre,
	const Color& col)
{
	DrawImage(rect.getX1(), rect.getY1(), 0.f, rect.getWidth(), rect.getHeight(), 
		pTexutre->getTexID(), 0, 1, 1, 0, col);
}


void DX11XRender::DrawImage(float xpos, float ypos, float z, float w, float h,
	texture::TexID texture_id, float s0, float t0, float s1, float t1, 
	const Colorf& col, bool filtered)
{
	float s[4], t[4];

	s[0] = s0;	t[0] = 1.0f - t0;
	s[1] = s1;	t[1] = 1.0f - t0;
	s[2] = s0;	t[2] = 1.0f - t1;
	s[3] = s1;	t[3] = 1.0f - t1;

	DrawImageWithUV(xpos, ypos, z, w, h, texture_id, s, t, col, filtered);
}

void DX11XRender::DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
	texture::TexID texture_id, const float* s, const float* t, 
	const Colorf& col, bool filtered)
{
	X_ASSERT_NOT_NULL(s);
	X_ASSERT_NOT_NULL(t);
	X_UNUSED(filtered);

	using namespace shader;

	//	SetCullMode(CullMode::NONE);
	// SetFFE(true);
	if (!SetFFE(shader::VertexFormat::P3F_T2F_C4B, true)) {
		return;
	}

	// Lock the entire buffer and obtain a pointer to the location where we have to
	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = xpos;
	Quad[0].pos.y = ypos;
	Quad[0].pos.z = z;
	// TR
	Quad[1].pos.x = xpos + w;
	Quad[1].pos.y = ypos;
	Quad[1].pos.z = z;
	// BL
	Quad[2].pos.x = xpos;
	Quad[2].pos.y = ypos + h;
	Quad[2].pos.z = z;
	// BR
	Quad[3].pos.x = xpos + w;
	Quad[3].pos.y = ypos + h;
	Quad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f(s[i], t[i]);
	}

	// We are finished with accessing the vertex buffer
	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	// Bind our vertex as the first data stream of our device
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	texture::XTexture::applyFromId(0, texture_id, 0);

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}


void DX11XRender::DrawQuad(float x, float y, float width, float height, const Color& col)
{
	DrawQuad(x, y, 0.f, width, height, col);
}

void DX11XRender::DrawQuad(float x, float y, float width, float height,
	const Color& col, const Color& borderCol)
{
	DrawQuad(x, y, 0.f, width, height, col);
	DrawRect(x, y, width, height, borderCol);
}

void DX11XRender::DrawQuad(float x, float y, float z, float width, float height,
	const Color& col, const Color& borderCol)
{
	DrawQuad(x, y, z, width, height, col);
	DrawRect(x, y, width, height, borderCol);
}

void DX11XRender::DrawQuad(float x, float y, float z, float width, float height, const Color& col)
{
	SetCullMode(CullMode::NONE);
	if (!SetFFE(shader::VertexFormat::P3F_T2F_C4B, false)) {
		return;
	}

	float fx = x;
	float fy = y;
	float fz = z;
	float fw = width;
	float fh = height;

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = fx;
	Quad[0].pos.y = fy;
	Quad[0].pos.z = fz;
	// TR
	Quad[1].pos.x = fx + fw;
	Quad[1].pos.y = fy;
	Quad[1].pos.z = fz;
	// BL
	Quad[2].pos.x = fx;
	Quad[2].pos.y = fy + fh;
	Quad[2].pos.z = fz;
	// BR
	Quad[3].pos.x = fx + fw;
	Quad[3].pos.y = fy + fh;
	Quad[3].pos.z = fz;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f::zero();
	}

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}

void DX11XRender::DrawQuad(Vec2<float> pos, float width, float height, const Color& col)
{
	DrawQuad(pos.x, pos.y, width, height, col);
}


void DX11XRender::DrawLines(Vec3f* points, uint32_t num, const Color& col)
{
	X_ASSERT_NOT_NULL(points);

	if (num < 2) // 2 points needed to make a line.
		return;

	SetCullMode(CullMode::NONE);

	uint32 nOffs, i;
	Vertex_P3F_T2F_C4B* Quad;

	Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(num, nOffs);

	for (i = 0; i < num; i++)
	{
		Quad[i].pos = points[i];
		Quad[i].color = col;
		Quad[i].st = Vec2<float32_t>::zero();
	}

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, num);
}


void DX11XRender::DrawLine(const Vec3f& pos1, const Vec3f& pos2)
{
	SetCullMode(CullMode::NONE);
//	SetFFE(false);

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(2, nOffs);

	Quad[0].pos = pos1;
	Quad[0].color = Color::white();
	Quad[0].st = Vec2f::zero();

	Quad[1].pos = pos2;
	Quad[1].color = Color::white();
	Quad[1].st = Vec2f::zero();

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, 2);
}


void DX11XRender::DrawLineColor(const Vec3f& pos1, const Color& color1,
	const Vec3f& pos2, const Color& color2)
{
//	SetFFE(false);
	if (!SetFFE(shader::VertexFormat::P3F_T2F_C4B, false)) {
		return;
	}

	// Lock the entire buffer and obtain a pointer to the location where we have to
	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(2, nOffs);

	Quad[0].pos = pos1;
	Quad[0].color = color1;
	Quad[1].pos = pos2;
	Quad[1].color = color2;

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, 2);
}

void DX11XRender::DrawRect(float x, float y, float width, float height, const Color& col)
{
	float x1 = x;
	float y1 = y;
	float x2 = x + width;
	float y2 = y + height;

	const Vec3f tl(x1, y1, 0);
	const Vec3f tr(x2, y1, 0);
	const Vec3f bl(x1, y2, 0);
	const Vec3f br(x2, y2, 0);

	// Top
	DrawLineColor(tl, col, tr, col);
	// bottom
	DrawLineColor(bl, col, br, col);
	// left down
	DrawLineColor(tl, col, bl, col);
	// right down
	DrawLineColor(tr, col, br, col);
}

void DX11XRender::DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
	float padding, uint32_t max)
{
	X_ASSERT_NOT_NULL(heights);
	X_ASSERT(num <= max, "Darw Chart has more items than max")(num, max);

	if (num < 1)
		return;

	// calculate the bar width.
	const float bar_width = ((rect.getWidth() / max) - padding) + padding / max;


	uint32 i, nOffs;
	Vertex_P3F_T2F_C4B* Quads = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(num * 6, nOffs);


	float right = rect.getX2();
	float bottom = rect.getY2();
	float height = rect.getHeight();
//	float width = rect.getWidth();

	Color8u col8(Col_Coral);


	// TL - TR - BR
	// BR - BL - TL
	for (i = 0; i < num; i++)
	{
		Vertex_P3F_T2F_C4B* Quad = &Quads[i * 6];
		float cur_bar = heights[i];

		// TL
		Quad[0].pos.x = right - bar_width;
		Quad[0].pos.y = bottom - (height * cur_bar);
		Quad[0].pos.z = 0.f;
		Quad[0].color = col8;

		// TR
		Quad[1].pos.x = right;
		Quad[1].pos.y = bottom - (height * cur_bar);
		Quad[1].pos.z = 0.f;
		Quad[1].color = col8;

		// BR
		Quad[2].pos.x = right;
		Quad[2].pos.y = bottom;
		Quad[2].pos.z = 0.f;
		Quad[2].color = col8;

		// BR
		Quad[3].pos.x = right;
		Quad[3].pos.y = bottom;
		Quad[3].pos.z = 0.f;
		Quad[3].color = col8;

		// BL
		Quad[4].pos.x = right - bar_width;
		Quad[4].pos.y = bottom;
		Quad[4].pos.z = 0.f;
		Quad[4].color = col8;

		// TL
		Quad[5].pos.x = right - bar_width;
		Quad[5].pos.y = bottom - (height * cur_bar);
		Quad[5].pos.z = 0.f;
		Quad[5].color = col8;

		right -= (bar_width + padding);
	}

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleList, nOffs, 6 * num);
}


void DX11XRender::DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
	PrimitiveTypePublic::Enum type)
{
	X_PROFILE_BEGIN("drawVB", core::profiler::SubSys::RENDER);

	X_ASSERT_NOT_NULL(pVertBuffer);

	if (size == 0)
		return;

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* pVertBuf;

	pVertBuf = (Vertex_P3F_T2F_C4B*)DynVB_[VertexPool::P3F_T2F_C4B].LockVB(size, nOffs);

	// copy data into gpu buffer.
	memcpy(pVertBuf, pVertBuffer, size * sizeof(Vertex_P3F_T2F_C4B));

	DynVB_[VertexPool::P3F_T2F_C4B].UnlockVB();
	DynVB_[VertexPool::P3F_T2F_C4B].Bind();

	if (!FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B, false))
		return;


	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveTypeToInternal(type), nOffs, size);
}


X_NAMESPACE_END