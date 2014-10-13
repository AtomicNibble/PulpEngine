#include "stdafx.h"
#include "Dx10Render.h"

#include "../Common/Textures/XTexture.h"


X_NAMESPACE_BEGIN(render)


void DX11XRender::RT_DrawLines(Vec3f* points, uint32_t num, const Colorf& col)
{
	X_ASSERT_NOT_NULL(points);

	SetCullMode(CullMode::NONE);

	uint32 nOffs, i;
	Vertex_P3F_C4B_T2F* Quad;

	Quad = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(num, nOffs);

	for (i = 0; i < num; i++)
	{
		Quad[i].pos = points[i];
		Quad[i].color = col;
		Quad[i].st = Vec2<float32_t>::zero();
	}

	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, num);
}


void DX11XRender::RT_DrawString(const Vec3f& pos, const char* pStr)
{
	X_ASSERT_NOT_NULL(pStr);




}



X_NAMESPACE_END