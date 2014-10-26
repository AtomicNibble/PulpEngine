#include "stdafx.h"
#include "Dx10Render.h"


X_NAMESPACE_BEGIN(render)



void DX11XRender::DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2,
const Vec3f& pos3, const Color& col)
{

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)m_DynVB[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

	Quad[0].pos = pos0;
	Quad[1].pos = pos1;
	Quad[2].pos = pos2;
	Quad[3].pos = pos3;


	for (uint32 i = 0; i < 4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f::zero();
	}

	m_DynVB[VertexPool::P3F_T2F_C4B].UnlockVB();
	m_DynVB[VertexPool::P3F_T2F_C4B].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}


void DX11XRender::Draw2dImage(float xpos, float ypos,
	float w, float h, texture::TexID texture_id, ColorT<float>& col)
{
	DrawImage(xpos, ypos, 0.f, w, h, texture_id,
		0, 1, 1, 0, col
		);
}


void DX11XRender::DrawImage(float xpos, float ypos, float z, float w, float h,
	int texture_id, float s0, float t0, float s1, float t1, const Colorf& col, bool filtered)
{
	float s[4], t[4];

	s[0] = s0;	t[0] = 1.0f - t0;
	s[1] = s1;	t[1] = 1.0f - t0;
	s[2] = s0;	t[2] = 1.0f - t1;
	s[3] = s1;	t[3] = 1.0f - t1;

	DrawImageWithUV(xpos, ypos, 0, w, h, texture_id, s, t, col, filtered);
}


void DX11XRender::DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
	int texture_id, float *s, float *t, const Colorf& col, bool filtered)
{
	X_ASSERT_NOT_NULL(s);
	X_ASSERT_NOT_NULL(t);

	rThread()->RC_DrawImageWithUV(xpos, ypos, z, w, h, texture_id, s, t, col, filtered);
}


void DX11XRender::RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
	int texture_id, float* s, float* t, const Colorf& col, bool filtered)
{
	using namespace shader;

	float fx = xpos;
	float fy = ypos;
	float fw = w;
	float fh = h;

	//	SetCullMode(CullMode::NONE);
	//	SetFFE(true);

	// Lock the entire buffer and obtain a pointer to the location where we have to
	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)m_DynVB[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

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
	m_DynVB[VertexPool::P3F_T2F_C4B].UnlockVB();


	/*
	XTexState state;
	state.setFilterMode(FilterMode::POINT);
	state.setClampMode(TextureAddressMode::MIRROR,
	TextureAddressMode::MIRROR, TextureAddressMode::MIRROR);

	// bind the texture.
	texture::XTexture::applyFromId(
	0,
	texture_id,
	texture::XTexture::getTexStateId(state)
	);
	*/

	// Bind our vertex as the first data stream of our device
	m_DynVB[VertexPool::P3F_T2F_C4B].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}


void DX11XRender::DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
	PrimitiveTypePublic::Enum type)
{
	X_PROFILE_BEGIN("drawVB", core::ProfileSubSys::RENDER);

	X_ASSERT_NOT_NULL(pVertBuffer);

	if (size == 0)
		return;

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* pVertBuf;

	pVertBuf = (Vertex_P3F_T2F_C4B*)m_DynVB[VertexPool::P3F_T2F_C4B].LockVB(size, nOffs);

	// copy data into gpu buffer.
	memcpy(pVertBuf, pVertBuffer, size * sizeof(Vertex_P3F_T2F_C4B));

	m_DynVB[VertexPool::P3F_T2F_C4B].UnlockVB();
	m_DynVB[VertexPool::P3F_T2F_C4B].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;


	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveTypeToInternal(type), nOffs, size);
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
	SetFFE(false);

	float fx = x;
	float fy = y;
	float fz = z;
	float fw = width;
	float fh = height;

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)m_DynVB[VertexPool::P3F_T2F_C4B].LockVB(4, nOffs);

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

	m_DynVB[VertexPool::P3F_T2F_C4B].UnlockVB();
	m_DynVB[VertexPool::P3F_T2F_C4B].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;

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

	rThread()->RC_DrawLines(points, num, col);
}


void DX11XRender::DrawLine(const Vec3f& pos1, const Vec3f& pos2)
{
	SetCullMode(CullMode::NONE);
	SetFFE(false);

	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)m_DynVB[VertexPool::P3F_T2F_C4B].LockVB(2, nOffs);

	Quad[0].pos = pos1;
	Quad[0].color = Color::white();
	Quad[0].st = Vec2f::zero();

	Quad[1].pos = pos2;
	Quad[1].color = Color::white();
	Quad[1].st = Vec2f::zero();

	m_DynVB[VertexPool::P3F_T2F_C4B].UnlockVB();
	m_DynVB[VertexPool::P3F_T2F_C4B].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, 2);
}


void DX11XRender::DrawLineColor(const Vec3f& pos1, const Color& color1,
	const Vec3f& pos2, const Color& vColor2)
{
	SetFFE(false);

	// Lock the entire buffer and obtain a pointer to the location where we have to
	uint32 nOffs;
	Vertex_P3F_T2F_C4B* Quad = (Vertex_P3F_T2F_C4B*)m_DynVB[VertexPool::P3F_T2F_C4B].LockVB(2, nOffs);

	Quad[0].pos = pos1;
	Quad[0].color = color1;
	Quad[1].pos = pos2;
	Quad[1].color = color1;

	m_DynVB[VertexPool::P3F_T2F_C4B].UnlockVB();
	m_DynVB[VertexPool::P3F_T2F_C4B].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, 2);
}

void DX11XRender::DrawRect(float x, float y, float width, float height, Color col)
{
	float x1 = x;
	float y1 = y;
	float x2 = x + width;
	float y2 = y + height;

	// Top
	DrawLineColor(Vec3f(x1, y1, 0), col, Vec3f(x2, y1, 0), col);
	// bottom
	DrawLineColor(Vec3f(x1, y2, 0), col, Vec3f(x2, y2, 0), col);
	// left down
	DrawLineColor(Vec3f(x1, y1, 0), col, Vec3f(x1, y2, 0), col);
	// right down
	DrawLineColor(Vec3f(x2, y1, 0), col, Vec3f(x2, y2, 0), col);
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
	Vertex_P3F_T2F_C4B* Quads = (Vertex_P3F_T2F_C4B*)m_DynVB[VertexPool::P3F_T2F_C4B].LockVB(num * 6, nOffs);


	float right = rect.getX2();
	float bottom = rect.getY2();
	float height = rect.getHeight();
	float width = rect.getWidth();

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

	m_DynVB[VertexPool::P3F_T2F_C4B].UnlockVB();
	m_DynVB[VertexPool::P3F_T2F_C4B].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleList, nOffs, 6 * num);
}


X_NAMESPACE_END