#include "stdafx.h"
#include "Dx10Render.h"

#include "../Common/Textures/XTexture.h"


X_NAMESPACE_BEGIN(render)


void DX11XRender::RT_DrawLines(Vec3f* points, uint32_t num, const Colorf& col)
{
	X_ASSERT_NOT_NULL(points);

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

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_T2F_C4B)))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, num);
}


void DX11XRender::RT_DrawString(const Vec3f& pos, const char* pStr)
{
	X_ASSERT_NOT_NULL(pStr);

	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(pos);
	X_UNUSED(pStr);
}



X_NAMESPACE_END