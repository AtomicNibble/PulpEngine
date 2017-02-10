#include "stdafx.h"
#include "AuxRender.h"

X_NAMESPACE_BEGIN(render)


namespace
{
	enum
	{
		AlphaBlended
	};


	X_INLINE uint32 AlphaFlags(const Color8u& col)
	{
		if (col.a > 0 && col.a < 0xFF) {
			return AlphaBlended;
		}
		return 0;
	}
}


// -------------------------------------------------------

RenderAux::AuxGeomCBRawData::AuxGeomCBRawData(core::MemoryArenaBase* arena) :
	auxPushArr(arena),
	auxVertexArr(arena),
	auxIndexArr(arena),
	auxDrawObjParamArr(arena),
	auxOrthoMatrices(arena)
{

}


void RenderAux::AuxGeomCBRawData::getSortedPushBuffer(size_t begin, size_t end,
	AuxSortedPushArr& auxSortedPushBuffer) const
{
	X_ASSERT(begin < end, "invalid range")(begin, end);
	X_ASSERT(end <= auxPushArr.size(), "invalid range")(end);

	auxSortedPushBuffer.reserve(end - begin);
	auxSortedPushBuffer.resize(0);


	for (const auto& ap : auxPushArr) {
		auxSortedPushBuffer.push_back(&ap);
	}

	std::sort(auxSortedPushBuffer.begin(), auxSortedPushBuffer.end(), 
		[](const AuxPushBufferEntry* lhs, const AuxPushBufferEntry* rhs)
		{
			if (lhs->renderFlags != rhs->renderFlags)
				return lhs->renderFlags < rhs->renderFlags;
			return lhs->transMatrixIdx < rhs->transMatrixIdx;
		}
	);
}

void RenderAux::AuxGeomCBRawData::reset(void)
{
	auxPushArr.clear();
	auxVertexArr.clear();
	auxIndexArr.clear();
	auxDrawObjParamArr.clear();
	auxOrthoMatrices.clear();
}

void RenderAux::AuxGeomCBRawData::free(void)
{
	auxPushArr.free();
	auxVertexArr.free();
	auxIndexArr.free();
	auxDrawObjParamArr.free();
	auxOrthoMatrices.free();
}

// -------------------------------------------------------

RenderAux::RenderAux(core::MemoryArenaBase* arena) :
	data_(arena)
{

}

RenderAux::~RenderAux()
{

}


void RenderAux::flush(void)
{

}

void RenderAux::clear(void)
{
	data_.reset();
}


// Lines

void RenderAux::drawLine(const Vec3f& v0, const Color8u& c0,
	const Vec3f& v1, const Color8u& c1, float thickness)
{
	if (thickness <= 1.0f)
	{
		XAuxVertex* pVertices = nullptr;
		addPrimitive(pVertices, 2, createLineRenderFlags(false) | AlphaFlags(c0) | AlphaFlags(c1));

		pVertices[0].pos = v0;
		pVertices[0].color = c0;
		pVertices[1].pos = v1;
		pVertices[1].color = c1;
	}
	else
	{
		drawThickLine(v0, c0, v1, c1, thickness);
	}
}

void RenderAux::drawLines(Vec3f* points, uint32_t numPoints,
	const Color8u& col, float thickness)
{
	if (thickness <= 1.0f)
	{
		XAuxVertex* pVertices = nullptr;
		addPrimitive(pVertices, numPoints, createLineRenderFlags(false) | AlphaFlags(col));

		for (uint32 i = 0; i < numPoints; ++i)
		{
			pVertices[i].pos = points[i];
			pVertices[i].color = col;
		}
	}
	else
	{
		for (uint32 i = 0; i < numPoints; i += 2)
		{
			drawThickLine(points[i], col, points[i + 1], col, thickness);
		}
	}
}

void RenderAux::drawLines(Vec3f* points, uint32_t numPoints,
	Color8u* col, float thickness)
{
	if (thickness <= 1.0f)
	{
		XAuxVertex* pVertices = nullptr;
		addPrimitive(pVertices, numPoints, createLineRenderFlags(false));

		for (uint32 i = 0; i < numPoints; ++i)
		{
			pVertices[i].pos = points[i];
			pVertices[i].color = col[i];
		}
	}
	else
	{
		for (uint32 i = 0; i < numPoints; i += 2)
		{
			drawThickLine(points[i], col[i], points[i + 1], col[i + 1], thickness);
		}
	}
}

void RenderAux::drawLines(Vec3f* points, uint32_t numPoints, uint16_t* indices,
	uint32_t numIndices, const Color8u& col, float thickness)
{
	if (thickness <= 1.0f)
	{
		XAuxVertex* pVertices = nullptr;
		uint16* pIndices = nullptr;
		addIndexedPrimitive(pVertices, numPoints, pIndices, numIndices, createLineRenderFlags(true) | AlphaFlags(col));

		for (uint32 i = 0; i < numPoints; ++i)
		{
			pVertices[i].pos = points[i];
			pVertices[i].color = col;
		}

		memcpy(pIndices, indices, sizeof(uint16) * numIndices);
	}
	else
	{
		for (uint32 i = 0; i < numIndices; i += 2)
		{
			drawThickLine(points[indices[i]], col, points[indices[i + 1]], col, thickness);
		}
	}
}

void RenderAux::drawLines(Vec3f* points, uint32_t numPoints, uint16_t* indices,
	uint32_t numIndices, Color8u* col, float thickness)
{
	if (thickness <= 1.0f)
	{
		XAuxVertex* pVertices = nullptr;
		uint16* pIndices = nullptr;
		addIndexedPrimitive(pVertices, numPoints, pIndices, numIndices, createLineRenderFlags(true));

		for (uint32 i = 0; i < numPoints; ++i)
		{
			pVertices[i].pos = points[i];
			pVertices[i].color = col[i];
		}

		memcpy(pIndices, indices, sizeof(uint16) * numIndices);
	}
	else
	{
		for (uint32 i = 0; i < numIndices; i += 2)
		{
			drawThickLine(points[indices[i]], col[indices[i]],
				points[indices[i + 1]], col[indices[i + 1]], thickness);
		}
	}

}


void RenderAux::drawThickLine(const Vec3f& v0, const Color8u& col0,
	const Vec3f& v1, const Color8u& col1, float thickness)
{
	// allocate space for two triangles
	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, 6, createTriangleRenderFlags(false));


	pVertices[0].pos = v0;
	pVertices[0].color = col0;
	pVertices[1].pos = v1;
	pVertices[1].color = col1;

	pVertices[2].pos = Vec3f(thickness, 0.0f, 0.0f);
}


// ---------------------------- Triangles ----------------------------


void RenderAux::drawTriangle(const Vec3f& v0, const Color8u& col0,
	const Vec3f& v1, const Color8u& col1,
	const Vec3f& v2, const Color8u& col2)
{
	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, 3, createTriangleRenderFlags(false));

	pVertices[0].pos = v0;
	pVertices[0].color = col0;

	pVertices[1].pos = v1;
	pVertices[1].color = col1;

	pVertices[2].pos = v2;
	pVertices[2].color = col2;
}

void RenderAux::drawTriangle(const Vec3f& v0, const Vec3f& v1,
	const Vec3f& v2, const Color8u& col)
{
	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, 3, createTriangleRenderFlags(false));

	pVertices[0].pos = v0;
	pVertices[0].color = col;

	pVertices[1].pos = v1;
	pVertices[1].color = col;

	pVertices[2].pos = v2;
	pVertices[2].color = col;
}


void RenderAux::drawTriangle(const Vec3f* points, uint32_t numPoints, const Color8u& c0)
{
	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, numPoints, createTriangleRenderFlags(false));

	Color8u color(c0);
	uint32 i;
	for (i = 0; i < numPoints; ++i)
	{
		pVertices[i].pos = points[i];
		pVertices[i].color = color;
	}
}

void RenderAux::drawTriangle(const Vec3f* points, uint32_t numPoints, const Color8u* pCol)
{
	if (numPoints == 0)
		return;

	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, numPoints, createTriangleRenderFlags(false));

	uint32 i;
	for (i = 0; i < numPoints; ++i)
	{
		pVertices[i].pos = points[i];
		pVertices[i].color = pCol[i];
	}
}

void RenderAux::drawTriangle(const Vec3f* pPoints, uint32_t numPoints,
	const uint16_t* indices, uint32_t numIndices, const Color8u& col)
{
	XAuxVertex* pVertices = nullptr;
	uint16* pIndices = nullptr;
	addIndexedPrimitive(pVertices, numPoints, pIndices, numIndices,
		createTriangleRenderFlags(true));

	Color8u color = col;
	uint32 i;
	for (i = 0; i < numPoints; ++i)
	{
		pVertices[i].pos = pPoints[i];
		pVertices[i].color = color;
	}

	memcpy(pIndices, indices, sizeof(uint16_t) * numIndices);
}

void RenderAux::drawTriangle(const Vec3f* pPoints, uint32_t numPoints,
	const uint16_t* indices, uint32_t numIndices, const Color8u* pCol)
{
	XAuxVertex* pVertices = nullptr;
	uint16* pIndices = nullptr;
	addIndexedPrimitive(pVertices, numPoints, pIndices, numIndices,
		createTriangleRenderFlags(true));

	for (uint32 i = 0; i < numPoints; ++i)
	{
		pVertices[i].pos = pPoints[i];
		pVertices[i].color = pCol[i];
	}

	memcpy(pIndices, indices, sizeof(uint16_t)* numIndices);
}

// ---------------------------- Point ----------------------------

void RenderAux::drawPoint(const Vec3f &pos, const Color8u& col, uint8_t size)
{
	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, 1, createPointRenderFlags(size));

	pVertices->pos = pos;
	pVertices->color = col;
}

void RenderAux::drawPoints(Vec3f* pPoints, uint32_t numPoints, const Color8u& col, uint8_t size)
{
	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, numPoints, createPointRenderFlags(size));

	for (uint32 i = 0; i < numPoints; ++i)
	{
		pVertices[i].pos = pPoints[i];
		pVertices[i].color = col;
	}
}

void RenderAux::drawPoints(Vec3f* pPoints, uint32_t numPoints, Color8u* pCol, uint8_t size)
{
	XAuxVertex* pVertices = nullptr;
	addPrimitive(pVertices, numPoints, createPointRenderFlags(size));

	for (uint32 i = 0; i < numPoints; ++i)
	{
		pVertices[i].pos = pPoints[i];
		pVertices[i].color = pCol[i];
	}
}


// ---------------------------- AABB ----------------------------

void RenderAux::drawAABB(const AABB& aabb, bool solid, const Color8u& col)
{
	XAuxVertex* pVertices = nullptr;
	uint16* pIndices = nullptr;

	if (!solid)
	{
		addIndexedPrimitive(pVertices, 8, pIndices, 24,
			createLineRenderFlags(true) | AlphaFlags(col));

		Color8u color = col;

		pVertices[0].pos = Vec3f(aabb.min.x, aabb.min.y, aabb.min.z);
		pVertices[0].color = color;
		pVertices[1].pos = Vec3f(aabb.min.x, aabb.max.y, aabb.min.z);
		pVertices[1].color = color;
		pVertices[2].pos = Vec3f(aabb.max.x, aabb.max.y, aabb.min.z);
		pVertices[2].color = color;
		pVertices[3].pos = Vec3f(aabb.max.x, aabb.min.y, aabb.min.z);
		pVertices[3].color = color;
		pVertices[4].pos = Vec3f(aabb.min.x, aabb.min.y, aabb.max.z);
		pVertices[4].color = color;
		pVertices[5].pos = Vec3f(aabb.min.x, aabb.max.y, aabb.max.z);
		pVertices[5].color = color;
		pVertices[6].pos = Vec3f(aabb.max.x, aabb.max.y, aabb.max.z);
		pVertices[6].color = color;
		pVertices[7].pos = Vec3f(aabb.max.x, aabb.min.y, aabb.max.z);
		pVertices[7].color = color;

		pIndices[0] = 0; pIndices[1] = 1;
		pIndices[2] = 1; pIndices[3] = 2;
		pIndices[4] = 2; pIndices[5] = 3;
		pIndices[6] = 3; pIndices[7] = 0;

		pIndices[8] = 4; pIndices[9] = 5;
		pIndices[10] = 5; pIndices[11] = 6;
		pIndices[12] = 6; pIndices[13] = 7;
		pIndices[14] = 7; pIndices[15] = 4;

		pIndices[16] = 0; pIndices[17] = 4;
		pIndices[18] = 1; pIndices[19] = 5;
		pIndices[20] = 2; pIndices[21] = 6;
		pIndices[22] = 3; pIndices[23] = 7;
	}
	else
	{
		addIndexedPrimitive(pVertices, 24, pIndices, 36, createTriangleRenderFlags(true));

		const Vec3f& xyz = aabb.min;
		const Vec3f xyZ(aabb.min.x, aabb.min.y, aabb.max.z);
		const Vec3f xYz(aabb.min.x, aabb.max.y, aabb.min.z);
		const Vec3f xYZ(aabb.min.x, aabb.max.y, aabb.max.z);
		const Vec3f Xyz(aabb.max.x, aabb.min.y, aabb.min.z);
		const Vec3f XyZ(aabb.max.x, aabb.min.y, aabb.max.z);
		const Vec3f XYz(aabb.max.x, aabb.max.y, aabb.min.z);
		const Vec3f& XYZ = aabb.max;

		Color8u colDown(col * 0.5f);

		pVertices[0].pos = xyz;
		pVertices[0].color = colDown;
		pVertices[1].pos = xYz;
		pVertices[1].color = colDown;
		pVertices[2].pos = XYz;
		pVertices[2].color = colDown;
		pVertices[3].pos = Xyz;
		pVertices[3].color = colDown;

		pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
		pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

		Color8u colTop(col);
		pVertices[4].pos = xyZ;
		pVertices[4].color = colTop;
		pVertices[5].pos = XyZ;
		pVertices[5].color = colTop;
		pVertices[6].pos = XYZ;
		pVertices[6].color = colTop;
		pVertices[7].pos = xYZ;
		pVertices[7].color = colTop;

		pIndices[6] = 4; pIndices[7] = 5; pIndices[8] = 6;
		pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

		Color8u colBack(col * 0.5f);
		pVertices[8].pos = xyz;
		pVertices[8].color = colBack;
		pVertices[9].pos = Xyz;
		pVertices[9].color = colBack;
		pVertices[10].pos = XyZ;
		pVertices[10].color = colBack;
		pVertices[11].pos = xyZ;
		pVertices[11].color = colBack;

		pIndices[12] = 8; pIndices[13] = 9; pIndices[14] = 10;
		pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

		Color8u colFront(col * 0.9f);
		pVertices[12].pos = xYz;
		pVertices[12].color = colFront;
		pVertices[13].pos = xYZ;
		pVertices[13].color = colFront;
		pVertices[14].pos = XYZ;
		pVertices[14].color = colFront;
		pVertices[15].pos = XYz;
		pVertices[15].color = colFront;

		pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
		pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

		Color8u colLeft(col * 0.7f);
		pVertices[16].pos = xyz;
		pVertices[16].color = colLeft;
		pVertices[17].pos = xyZ;
		pVertices[17].color = colLeft;
		pVertices[18].pos = xYZ;
		pVertices[18].color = colLeft;
		pVertices[19].pos = xYz;
		pVertices[19].color = colLeft;

		pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
		pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

		Color8u colRight(col * 0.8f);
		pVertices[20].pos = Xyz;
		pVertices[20].color = colRight;
		pVertices[21].pos = XYz;
		pVertices[21].color = colRight;
		pVertices[22].pos = XYZ;
		pVertices[22].color = colRight;
		pVertices[23].pos = XyZ;
		pVertices[23].color = colRight;

		pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
		pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;
	}
}

void RenderAux::drawAABB(const AABB& aabb, const Vec3f& pos,
	bool solid, const Color8u& col)
{
	XAuxVertex* pVertices = nullptr;
	uint16* pIndices = nullptr;

	if (!solid)
	{
		addIndexedPrimitive(pVertices, 8, pIndices, 24,
			createLineRenderFlags(true) | AlphaFlags(col));

		Color8u color(col);

		pVertices[0].pos = Vec3f(aabb.min.x, aabb.min.y, aabb.min.z) + pos;
		pVertices[0].color = color;
		pVertices[1].pos = Vec3f(aabb.min.x, aabb.max.y, aabb.min.z) + pos;
		pVertices[1].color = color;
		pVertices[2].pos = Vec3f(aabb.max.x, aabb.max.y, aabb.min.z) + pos;
		pVertices[2].color = color;
		pVertices[3].pos = Vec3f(aabb.max.x, aabb.min.y, aabb.min.z) + pos;
		pVertices[3].color = color;
		pVertices[4].pos = Vec3f(aabb.min.x, aabb.min.y, aabb.max.z) + pos;
		pVertices[4].color = color;
		pVertices[5].pos = Vec3f(aabb.min.x, aabb.max.y, aabb.max.z) + pos;
		pVertices[5].color = color;
		pVertices[6].pos = Vec3f(aabb.max.x, aabb.max.y, aabb.max.z) + pos;
		pVertices[6].color = color;
		pVertices[7].pos = Vec3f(aabb.max.x, aabb.min.y, aabb.max.z) + pos;
		pVertices[7].color = color;

		pIndices[0] = 0; pIndices[1] = 1;
		pIndices[2] = 1; pIndices[3] = 2;
		pIndices[4] = 2; pIndices[5] = 3;
		pIndices[6] = 3; pIndices[7] = 0;

		pIndices[8] = 4; pIndices[9] = 5;
		pIndices[10] = 5; pIndices[11] = 6;
		pIndices[12] = 6; pIndices[13] = 7;
		pIndices[14] = 7; pIndices[15] = 4;

		pIndices[16] = 0; pIndices[17] = 4;
		pIndices[18] = 1; pIndices[19] = 5;
		pIndices[20] = 2; pIndices[21] = 6;
		pIndices[22] = 3; pIndices[23] = 7;
	}
	else
	{
		addIndexedPrimitive(pVertices, 24, pIndices, 36, createTriangleRenderFlags(true));

		Vec3f xyz(Vec3f(aabb.min.x, aabb.min.y, aabb.min.z) + pos);
		Vec3f xyZ(Vec3f(aabb.min.x, aabb.min.y, aabb.max.z) + pos);
		Vec3f xYz(Vec3f(aabb.min.x, aabb.max.y, aabb.min.z) + pos);
		Vec3f xYZ(Vec3f(aabb.min.x, aabb.max.y, aabb.max.z) + pos);
		Vec3f Xyz(Vec3f(aabb.max.x, aabb.min.y, aabb.min.z) + pos);
		Vec3f XyZ(Vec3f(aabb.max.x, aabb.min.y, aabb.max.z) + pos);
		Vec3f XYz(Vec3f(aabb.max.x, aabb.max.y, aabb.min.z) + pos);
		Vec3f XYZ(Vec3f(aabb.max.x, aabb.max.y, aabb.max.z) + pos);

		Color8u colDown(col * 0.5f);

		pVertices[0].pos = xyz;
		pVertices[0].color = colDown;
		pVertices[1].pos = xYz;
		pVertices[1].color = colDown;
		pVertices[2].pos = XYz;
		pVertices[2].color = colDown;
		pVertices[3].pos = Xyz;
		pVertices[3].color = colDown;

		pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
		pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

		Color8u colTop(col);
		pVertices[4].pos = xyZ;
		pVertices[4].color = colTop;
		pVertices[5].pos = XyZ;
		pVertices[5].color = colTop;
		pVertices[6].pos = XYZ;
		pVertices[6].color = colTop;
		pVertices[7].pos = xYZ;
		pVertices[7].color = colTop;

		pIndices[6] = 4; pIndices[7] = 5; pIndices[8] = 6;
		pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

		Color8u colBack(col * 0.5f);
		pVertices[8].pos = xyz;
		pVertices[8].color = colBack;
		pVertices[9].pos = Xyz;
		pVertices[9].color = colBack;
		pVertices[10].pos = XyZ;
		pVertices[10].color = colBack;
		pVertices[11].pos = xyZ;
		pVertices[11].color = colBack;

		pIndices[12] = 8; pIndices[13] = 9; pIndices[14] = 10;
		pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

		Color8u colFront(col * 0.9f);
		pVertices[12].pos = xYz;
		pVertices[12].color = colFront;
		pVertices[13].pos = xYZ;
		pVertices[13].color = colFront;
		pVertices[14].pos = XYZ;
		pVertices[14].color = colFront;
		pVertices[15].pos = XYz;
		pVertices[15].color = colFront;

		pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
		pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

		Color8u colLeft(col * 0.7f);
		pVertices[16].pos = xyz;
		pVertices[16].color = colLeft;
		pVertices[17].pos = xyZ;
		pVertices[17].color = colLeft;
		pVertices[18].pos = xYZ;
		pVertices[18].color = colLeft;
		pVertices[19].pos = xYz;
		pVertices[19].color = colLeft;

		pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
		pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

		Color8u colRight(col * 0.8f);
		pVertices[20].pos = Xyz;
		pVertices[20].color = colRight;
		pVertices[21].pos = XYz;
		pVertices[21].color = colRight;
		pVertices[22].pos = XYZ;
		pVertices[22].color = colRight;
		pVertices[23].pos = XyZ;
		pVertices[23].color = colRight;

		pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
		pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;
	}

}

void RenderAux::drawAABB(const AABB& aabb, const Matrix34f& matWorld,
	bool solid, const Color8u& col)
{
	XAuxVertex* pVertices = nullptr;
	uint16* pIndices = nullptr;

	if (!solid)
	{
		addIndexedPrimitive(pVertices, 8, pIndices, 24,
			createLineRenderFlags(true) | AlphaFlags(col));

		Color8u color(col);

		pVertices[0].pos = matWorld * Vec3f(aabb.min.x, aabb.min.y, aabb.min.z);
		pVertices[0].color = color;
		pVertices[1].pos = matWorld * Vec3f(aabb.min.x, aabb.max.y, aabb.min.z);
		pVertices[1].color = color;
		pVertices[2].pos = matWorld * Vec3f(aabb.max.x, aabb.max.y, aabb.min.z);
		pVertices[2].color = color;
		pVertices[3].pos = matWorld * Vec3f(aabb.max.x, aabb.min.y, aabb.min.z);
		pVertices[3].color = color;
		pVertices[4].pos = matWorld * Vec3f(aabb.min.x, aabb.min.y, aabb.max.z);
		pVertices[4].color = color;
		pVertices[5].pos = matWorld * Vec3f(aabb.min.x, aabb.max.y, aabb.max.z);
		pVertices[5].color = color;
		pVertices[6].pos = matWorld * Vec3f(aabb.max.x, aabb.max.y, aabb.max.z);
		pVertices[6].color = color;
		pVertices[7].pos = matWorld * Vec3f(aabb.max.x, aabb.min.y, aabb.max.z);
		pVertices[7].color = color;

		pIndices[0] = 0; pIndices[1] = 1;
		pIndices[2] = 1; pIndices[3] = 2;
		pIndices[4] = 2; pIndices[5] = 3;
		pIndices[6] = 3; pIndices[7] = 0;

		pIndices[8] = 4; pIndices[9] = 5;
		pIndices[10] = 5; pIndices[11] = 6;
		pIndices[12] = 6; pIndices[13] = 7;
		pIndices[14] = 7; pIndices[15] = 4;

		pIndices[16] = 0; pIndices[17] = 4;
		pIndices[18] = 1; pIndices[19] = 5;
		pIndices[20] = 2; pIndices[21] = 6;
		pIndices[22] = 3; pIndices[23] = 7;
	}
	else
	{
		addIndexedPrimitive(pVertices, 24, pIndices, 36, createTriangleRenderFlags(true));

		Vec3f xyz(matWorld * Vec3f(aabb.min.x, aabb.min.y, aabb.min.z));
		Vec3f xyZ(matWorld * Vec3f(aabb.min.x, aabb.min.y, aabb.max.z));
		Vec3f xYz(matWorld * Vec3f(aabb.min.x, aabb.max.y, aabb.min.z));
		Vec3f xYZ(matWorld * Vec3f(aabb.min.x, aabb.max.y, aabb.max.z));
		Vec3f Xyz(matWorld * Vec3f(aabb.max.x, aabb.min.y, aabb.min.z));
		Vec3f XyZ(matWorld * Vec3f(aabb.max.x, aabb.min.y, aabb.max.z));
		Vec3f XYz(matWorld * Vec3f(aabb.max.x, aabb.max.y, aabb.min.z));
		Vec3f XYZ(matWorld * Vec3f(aabb.max.x, aabb.max.y, aabb.max.z));

		Color8u colDown(col * 0.5f);

		pVertices[0].pos = xyz;
		pVertices[0].color = colDown;
		pVertices[1].pos = xYz;
		pVertices[1].color = colDown;
		pVertices[2].pos = XYz;
		pVertices[2].color = colDown;
		pVertices[3].pos = Xyz;
		pVertices[3].color = colDown;

		pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
		pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

		Color8u colTop(col);
		pVertices[4].pos = xyZ;
		pVertices[4].color = colTop;
		pVertices[5].pos = XyZ;
		pVertices[5].color = colTop;
		pVertices[6].pos = XYZ;
		pVertices[6].color = colTop;
		pVertices[7].pos = xYZ;
		pVertices[7].color = colTop;

		pIndices[6] = 4; pIndices[7] = 5; pIndices[8] = 6;
		pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

		Color8u colBack(col * 0.5f);
		pVertices[8].pos = xyz;
		pVertices[8].color = colBack;
		pVertices[9].pos = Xyz;
		pVertices[9].color = colBack;
		pVertices[10].pos = XyZ;
		pVertices[10].color = colBack;
		pVertices[11].pos = xyZ;
		pVertices[11].color = colBack;

		pIndices[12] = 8; pIndices[13] = 9; pIndices[14] = 10;
		pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

		Color8u colFront(col * 0.9f);
		pVertices[12].pos = xYz;
		pVertices[12].color = colFront;
		pVertices[13].pos = xYZ;
		pVertices[13].color = colFront;
		pVertices[14].pos = XYZ;
		pVertices[14].color = colFront;
		pVertices[15].pos = XYz;
		pVertices[15].color = colFront;

		pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
		pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

		Color8u colLeft(col * 0.7f);
		pVertices[16].pos = xyz;
		pVertices[16].color = colLeft;
		pVertices[17].pos = xyZ;
		pVertices[17].color = colLeft;
		pVertices[18].pos = xYZ;
		pVertices[18].color = colLeft;
		pVertices[19].pos = xYz;
		pVertices[19].color = colLeft;

		pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
		pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

		Color8u colRight(col * 0.8f);
		pVertices[20].pos = Xyz;
		pVertices[20].color = colRight;
		pVertices[21].pos = XYz;
		pVertices[21].color = colRight;
		pVertices[22].pos = XYZ;
		pVertices[22].color = colRight;
		pVertices[23].pos = XyZ;
		pVertices[23].color = colRight;

		pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
		pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;
	}
}

// ---------------------------- OBB ----------------------------

void RenderAux::drawOBB(const OBB& obb, const Vec3f& pos,
	bool solid, const Color8u& col)
{
	XAuxVertex* pVertices = nullptr;
	uint16* pIndices = nullptr;

	if (false == solid)
	{
		addIndexedPrimitive(pVertices, 8, pIndices, 24, createLineRenderFlags(true) | AlphaFlags(col));

		Color8u color(col);

		AABB aabb(obb.center() - obb.halfVec(), obb.center() + obb.halfVec());
		pVertices[0].pos = obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.min.z) + pos;
		pVertices[0].color = color;
		pVertices[1].pos = obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.min.z) + pos;
		pVertices[1].color = color;
		pVertices[2].pos = obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.min.z) + pos;
		pVertices[2].color = color;
		pVertices[3].pos = obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.min.z) + pos;
		pVertices[3].color = color;
		pVertices[4].pos = obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.max.z) + pos;
		pVertices[4].color = color;
		pVertices[5].pos = obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.max.z) + pos;
		pVertices[5].color = color;
		pVertices[6].pos = obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.max.z) + pos;
		pVertices[6].color = color;
		pVertices[7].pos = obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.max.z) + pos;
		pVertices[7].color = color;

		pIndices[0] = 0; pIndices[1] = 1;
		pIndices[2] = 1; pIndices[3] = 2;
		pIndices[4] = 2; pIndices[5] = 3;
		pIndices[6] = 3; pIndices[7] = 0;

		pIndices[8] = 4; pIndices[9] = 5;
		pIndices[10] = 5; pIndices[11] = 6;
		pIndices[12] = 6; pIndices[13] = 7;
		pIndices[14] = 7; pIndices[15] = 4;

		pIndices[16] = 0; pIndices[17] = 4;
		pIndices[18] = 1; pIndices[19] = 5;
		pIndices[20] = 2; pIndices[21] = 6;
		pIndices[22] = 3; pIndices[23] = 7;
	}
	else
	{
		addIndexedPrimitive(pVertices, 24, pIndices, 36, createTriangleRenderFlags(true));

		AABB aabb(obb.center() - obb.halfVec(), obb.center() + obb.halfVec());
		Vec3f xyz(obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.min.z) + pos);
		Vec3f xyZ(obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.max.z) + pos);
		Vec3f xYz(obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.min.z) + pos);
		Vec3f xYZ(obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.max.z) + pos);
		Vec3f Xyz(obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.min.z) + pos);
		Vec3f XyZ(obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.max.z) + pos);
		Vec3f XYz(obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.min.z) + pos);
		Vec3f XYZ(obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.max.z) + pos);

		Color8u colDown(0.5f * col);
		pVertices[0].pos = xyz;
		pVertices[0].color = colDown;
		pVertices[1].pos = xYz;
		pVertices[1].color = colDown;
		pVertices[2].pos = XYz;
		pVertices[2].color = colDown;
		pVertices[3].pos = Xyz;
		pVertices[3].color = colDown;

		pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
		pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

		Color8u colTop(col);
		pVertices[4].pos = xyZ;
		pVertices[4].color = colTop;
		pVertices[5].pos = XyZ;
		pVertices[5].color = colTop;
		pVertices[6].pos = XYZ;
		pVertices[6].color = colTop;
		pVertices[7].pos = xYZ;
		pVertices[7].color = colTop;

		pIndices[6] = 4; pIndices[7] = 5; pIndices[8] = 6;
		pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

		Color8u colBack(0.6f * col);
		pVertices[8].pos = xyz;
		pVertices[8].color = colBack;
		pVertices[9].pos = Xyz;
		pVertices[9].color = colBack;
		pVertices[10].pos = XyZ;
		pVertices[10].color = colBack;
		pVertices[11].pos = xyZ;
		pVertices[11].color = colBack;

		pIndices[12] = 8; pIndices[13] = 9; pIndices[14] = 10;
		pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

		Color8u colFront(0.9f * col);
		pVertices[12].pos = xYz;
		pVertices[12].color = colFront;
		pVertices[13].pos = xYZ;
		pVertices[13].color = colFront;
		pVertices[14].pos = XYZ;
		pVertices[14].color = colFront;
		pVertices[15].pos = XYz;
		pVertices[15].color = colFront;

		pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
		pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

		Color8u colLeft(0.7f * col);
		pVertices[16].pos = xyz;
		pVertices[16].color = colLeft;
		pVertices[17].pos = xyZ;
		pVertices[17].color = colLeft;
		pVertices[18].pos = xYZ;
		pVertices[18].color = colLeft;
		pVertices[19].pos = xYz;
		pVertices[19].color = colLeft;

		pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
		pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

		Color8u colRight(0.8f * col);
		pVertices[20].pos = Xyz;
		pVertices[20].color = colRight;
		pVertices[21].pos = XYz;
		pVertices[21].color = colRight;
		pVertices[22].pos = XYZ;
		pVertices[22].color = colRight;
		pVertices[23].pos = XyZ;
		pVertices[23].color = colRight;

		pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
		pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;
	}


}


void RenderAux::drawOBB(const OBB& obb, const Matrix34f& matWorld,
	bool solid, const Color8u& col)
{

	XAuxVertex* pVertices = nullptr;
	uint16* pIndices = nullptr;

	if (false == solid)
	{
		addIndexedPrimitive(pVertices, 8, pIndices, 24, createLineRenderFlags(true) | AlphaFlags(col));

		Color8u color(col);

		AABB aabb(obb.center() - obb.halfVec(), obb.center() + obb.halfVec());
		pVertices[0].pos = matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.min.z));
		pVertices[0].color = color;
		pVertices[1].pos = matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.min.z));
		pVertices[1].color = color;
		pVertices[2].pos = matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.min.z));
		pVertices[2].color = color;
		pVertices[3].pos = matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.min.z));
		pVertices[3].color = color;
		pVertices[4].pos = matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.max.z));
		pVertices[4].color = color;
		pVertices[5].pos = matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.max.z));
		pVertices[5].color = color;
		pVertices[6].pos = matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.max.z));
		pVertices[6].color = color;
		pVertices[7].pos = matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.max.z));
		pVertices[7].color = color;

		pIndices[0] = 0; pIndices[1] = 1;
		pIndices[2] = 1; pIndices[3] = 2;
		pIndices[4] = 2; pIndices[5] = 3;
		pIndices[6] = 3; pIndices[7] = 0;

		pIndices[8] = 4; pIndices[9] = 5;
		pIndices[10] = 5; pIndices[11] = 6;
		pIndices[12] = 6; pIndices[13] = 7;
		pIndices[14] = 7; pIndices[15] = 4;

		pIndices[16] = 0; pIndices[17] = 4;
		pIndices[18] = 1; pIndices[19] = 5;
		pIndices[20] = 2; pIndices[21] = 6;
		pIndices[22] = 3; pIndices[23] = 7;
	}
	else
	{
		addIndexedPrimitive(pVertices, 24, pIndices, 36, createTriangleRenderFlags(true));

		AABB aabb(obb.center() - obb.halfVec(), obb.center() + obb.halfVec());
		Vec3f xyz(matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.min.z)));
		Vec3f xyZ(matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.min.y, aabb.max.z)));
		Vec3f xYz(matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.min.z)));
		Vec3f xYZ(matWorld * (obb.orientation() * Vec3f(aabb.min.x, aabb.max.y, aabb.max.z)));
		Vec3f Xyz(matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.min.z)));
		Vec3f XyZ(matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.min.y, aabb.max.z)));
		Vec3f XYz(matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.min.z)));
		Vec3f XYZ(matWorld * (obb.orientation() * Vec3f(aabb.max.x, aabb.max.y, aabb.max.z)));

		Color8u colDown(0.5f * col);
		pVertices[0].pos = xyz;
		pVertices[0].color = colDown;
		pVertices[1].pos = xYz;
		pVertices[1].color = colDown;
		pVertices[2].pos = XYz;
		pVertices[2].color = colDown;
		pVertices[3].pos = Xyz;
		pVertices[3].color = colDown;

		pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
		pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

		Color8u colTop(col);
		pVertices[4].pos = xyZ;
		pVertices[4].color = colTop;
		pVertices[5].pos = XyZ;
		pVertices[5].color = colTop;
		pVertices[6].pos = XYZ;
		pVertices[6].color = colTop;
		pVertices[7].pos = xYZ;
		pVertices[7].color = colTop;

		pIndices[6] = 4; pIndices[7] = 5; pIndices[8] = 6;
		pIndices[9] = 4; pIndices[10] = 6; pIndices[11] = 7;

		Color8u colBack(0.6f * col);
		pVertices[8].pos = xyz;
		pVertices[8].color = colBack;
		pVertices[9].pos = Xyz;
		pVertices[9].color = colBack;
		pVertices[10].pos = XyZ;
		pVertices[10].color = colBack;
		pVertices[11].pos = xyZ;
		pVertices[11].color = colBack;

		pIndices[12] = 8; pIndices[13] = 9; pIndices[14] = 10;
		pIndices[15] = 8; pIndices[16] = 10; pIndices[17] = 11;

		Color8u colFront(0.9f * col);
		pVertices[12].pos = xYz;
		pVertices[12].color = colFront;
		pVertices[13].pos = xYZ;
		pVertices[13].color = colFront;
		pVertices[14].pos = XYZ;
		pVertices[14].color = colFront;
		pVertices[15].pos = XYz;
		pVertices[15].color = colFront;

		pIndices[18] = 12; pIndices[19] = 13; pIndices[20] = 14;
		pIndices[21] = 12; pIndices[22] = 14; pIndices[23] = 15;

		Color8u colLeft(0.7f * col);
		pVertices[16].pos = xyz;
		pVertices[16].color = colLeft;
		pVertices[17].pos = xyZ;
		pVertices[17].color = colLeft;
		pVertices[18].pos = xYZ;
		pVertices[18].color = colLeft;
		pVertices[19].pos = xYz;
		pVertices[19].color = colLeft;

		pIndices[24] = 16; pIndices[25] = 17; pIndices[26] = 18;
		pIndices[27] = 16; pIndices[28] = 18; pIndices[29] = 19;

		Color8u colRight(0.8f * col);
		pVertices[20].pos = Xyz;
		pVertices[20].color = colRight;
		pVertices[21].pos = XYz;
		pVertices[21].color = colRight;
		pVertices[22].pos = XYZ;
		pVertices[22].color = colRight;
		pVertices[23].pos = XyZ;
		pVertices[23].color = colRight;

		pIndices[30] = 20; pIndices[31] = 21; pIndices[32] = 22;
		pIndices[33] = 20; pIndices[34] = 22; pIndices[35] = 23;
	}

}


// ---------------------------- Sphere ----------------------------


void RenderAux::drawSphere(const Sphere& sphere, const Color8u& col,
	bool drawShaded)
{
	if (sphere.radius() > 0.0f)
	{
		AuxDrawObjParams* pDrawParams = nullptr;
		addObject(pDrawParams, createObjectRenderFlags(DrawObjType::Sphere));

		Matrix33f scale = Matrix33f::createScale(sphere.radius());
		Matrix34f trans = Matrix34f::createTranslation(sphere.center());

		pDrawParams->matWorld = trans * scale;
		pDrawParams->color = col;
		pDrawParams->size = sphere.radius();
		pDrawParams->shaded = drawShaded;
	}
}


void RenderAux::drawSphere(const Sphere& sphere, const Matrix34f& mat,
	const Color8u& col, bool drawShaded)
{
	if (sphere.radius() > 0.0f)
	{
		AuxDrawObjParams* pDrawParams = nullptr;
		addObject(pDrawParams, createObjectRenderFlags(DrawObjType::Sphere) | AlphaFlags(col));


		Matrix33f scale = Matrix33f::createScale(sphere.radius());
		Matrix34f trans = Matrix34f::createTranslation(mat * sphere.center());

		pDrawParams->matWorld = trans * scale;
		pDrawParams->color = col;
		pDrawParams->size = sphere.radius();
		pDrawParams->shaded = drawShaded;
	}
}


// ---------------------------- Sphere ----------------------------

void RenderAux::drawCone(const Vec3f& pos, const Vec3f& dir, float radius,
	float height, const Color8u& col, bool drawShaded)
{
	if (radius > 0.0f && height > 0.0f && dir.lengthSquared() > 0.0f)
	{
		AuxDrawObjParams* pDrawParams = nullptr;
		addObject(pDrawParams, createObjectRenderFlags(DrawObjType::Cone));

		Vec3f direction(dir.normalized());
		Vec3f orthogonal(direction.getOrthogonal().normalized());

		Matrix33f matRot;
		matRot.setToIdentity();
		matRot.setColumn(0, orthogonal);
		matRot.setColumn(1, direction);
		matRot.setColumn(2, orthogonal.cross(direction));


		Matrix33f scale = Matrix33f::createScale(Vec3f(radius, height, radius));
		Matrix34f trans = Matrix34f::createTranslation(pos);

		pDrawParams->matWorld = trans * matRot * scale;
		pDrawParams->color = col;
		pDrawParams->size = core::Max(radius, height * 0.5f);
		pDrawParams->shaded = drawShaded;
	}

}

// ---------------------------- Cylinder ----------------------------

void RenderAux::drawCylinder(const Vec3f& pos, const Vec3f& dir, float radius,
	float height, const Color8u& col, bool drawShaded)
{

	if (radius > 0.0f && height > 0.0f && dir.lengthSquared() > 0.0f)
	{
		AuxDrawObjParams* pDrawParams = nullptr;
		addObject(pDrawParams, createObjectRenderFlags(DrawObjType::Cylinder));

		Vec3f direction(dir.normalized());
		Vec3f orthogonal(direction.getOrthogonal().normalized());

		Matrix33f matRot;
		matRot.setToIdentity();
		matRot.setColumn(0, orthogonal);
		matRot.setColumn(1, direction);
		matRot.setColumn(2, orthogonal.cross(direction));

		Matrix33f scale = Matrix33f::createScale(Vec3f(radius, height, radius));
		Matrix34f trans = Matrix34f::createTranslation(pos);

		pDrawParams->matWorld = trans * matRot * scale;
		pDrawParams->color = col;
		pDrawParams->size = core::Max(radius, height * 0.5f);
		pDrawParams->shaded = drawShaded;
	}

}


// ---------------------------- Bone ----------------------------


void RenderAux::drawBone(const Transformf& rParent, const Transformf& rChild, const Color8u& col)
{
	//	X_ASSERT_NOT_IMPLEMENTED();

	Vec3f p = rParent.getTranslation();
	Vec3f c = rChild.getTranslation();
	Vec3f vBoneVec = c - p;
	float fBoneLength = vBoneVec.length();

	if (fBoneLength < 1e-4) {
		return;
	}

	Matrix33f m33 = Matrix33f::createRotationV01(Vec3f(1, 0, 0), vBoneVec / fBoneLength);
	Matrix34f m34 = Matrix34f(m33, p);

	float32_t t = fBoneLength*0.025f;

	//bone points in x-direction
	Vec3f s = Vec3f::zero();
	Vec3f m0 = Vec3f(t, +t, +t);
	Vec3f m1 = Vec3f(t, -t, +t);
	Vec3f m2 = Vec3f(t, -t, -t);
	Vec3f m3 = Vec3f(t, +t, -t);
	Vec3f e = Vec3f(fBoneLength, 0, 0);

	Vec3f VBuffer[6];
	Color8u CBuffer[6];

	Color8u comp;
	comp.r = col.r ^ 0xFF;
	comp.r = (col.r - comp.r) / 2;
	comp.g = col.g ^ 0xFF;
	comp.g = (col.g - comp.g) / 2;
	comp.b = col.b ^ 0xFF;
	comp.b = (col.b - comp.b) / 2;

	comp.a = col.a;

	VBuffer[0] = m34*s;
	CBuffer[0] = comp;  //start of bone (joint)

	VBuffer[1] = m34*m0;
	CBuffer[1] = col;
	VBuffer[2] = m34*m1;
	CBuffer[2] = col;
	VBuffer[3] = m34*m2;
	CBuffer[3] = col;
	VBuffer[4] = m34*m3;
	CBuffer[4] = col;

	VBuffer[5] = m34*e;
	CBuffer[5] = comp; //end of bone


	drawLine(VBuffer[0], CBuffer[0], VBuffer[1], CBuffer[1]);
	drawLine(VBuffer[0], CBuffer[0], VBuffer[2], CBuffer[2]);
	drawLine(VBuffer[0], CBuffer[0], VBuffer[3], CBuffer[3]);
	drawLine(VBuffer[0], CBuffer[0], VBuffer[4], CBuffer[4]);

	drawLine(VBuffer[1], CBuffer[1], VBuffer[2], CBuffer[2]);
	drawLine(VBuffer[2], CBuffer[2], VBuffer[3], CBuffer[3]);
	drawLine(VBuffer[3], CBuffer[3], VBuffer[4], CBuffer[4]);
	drawLine(VBuffer[4], CBuffer[4], VBuffer[1], CBuffer[1]);

	drawLine(VBuffer[5], CBuffer[5], VBuffer[1], CBuffer[1]);
	drawLine(VBuffer[5], CBuffer[5], VBuffer[2], CBuffer[2]);
	drawLine(VBuffer[5], CBuffer[5], VBuffer[3], CBuffer[3]);
	drawLine(VBuffer[5], CBuffer[5], VBuffer[4], CBuffer[4]);
}

void RenderAux::drawBone(const Matrix34f& rParent, const Matrix34f& rBone, const Color8u& col)
{
	Vec3f p = rParent.getTranslate();
	Vec3f c = rBone.getTranslate();
	Vec3f vBoneVec = c - p;
	float fBoneLength = vBoneVec.length();

	if (fBoneLength < 1e-4) {
		return;
	}

	Matrix33f m33 = Matrix33f::createRotationV01(Vec3f(1, 0, 0), vBoneVec / fBoneLength);
	Matrix34f m34 = Matrix34f(m33, p);

	float32_t t = fBoneLength*0.025f;

	//bone points in x-direction
	Vec3f s = Vec3f::zero();
	Vec3f m0 = Vec3f(t, +t, +t);
	Vec3f m1 = Vec3f(t, -t, +t);
	Vec3f m2 = Vec3f(t, -t, -t);
	Vec3f m3 = Vec3f(t, +t, -t);
	Vec3f e = Vec3f(fBoneLength, 0, 0);

	Vec3f VBuffer[6];
	Color8u CBuffer[6];

	Color8u comp;
	comp.r = col.r ^ 0xFF;
	comp.r = (col.r - comp.r) / 2;
	comp.g = col.g ^ 0xFF;
	comp.g = (col.g - comp.g) / 2;
	comp.b = col.b ^ 0xFF;
	comp.b = (col.b - comp.b) / 2;

	comp.a = col.a;

	VBuffer[0] = m34*s;
	CBuffer[0] = comp;  //start of bone (joint)

	VBuffer[1] = m34*m0;
	CBuffer[1] = col;
	VBuffer[2] = m34*m1;
	CBuffer[2] = col;
	VBuffer[3] = m34*m2;
	CBuffer[3] = col;
	VBuffer[4] = m34*m3;
	CBuffer[4] = col;

	VBuffer[5] = m34*e;
	CBuffer[5] = comp; //end of bone


	drawLine(VBuffer[0], CBuffer[0], VBuffer[1], CBuffer[1]);
	drawLine(VBuffer[0], CBuffer[0], VBuffer[2], CBuffer[2]);
	drawLine(VBuffer[0], CBuffer[0], VBuffer[3], CBuffer[3]);
	drawLine(VBuffer[0], CBuffer[0], VBuffer[4], CBuffer[4]);

	drawLine(VBuffer[1], CBuffer[1], VBuffer[2], CBuffer[2]);
	drawLine(VBuffer[2], CBuffer[2], VBuffer[3], CBuffer[3]);
	drawLine(VBuffer[3], CBuffer[3], VBuffer[4], CBuffer[4]);
	drawLine(VBuffer[4], CBuffer[4], VBuffer[1], CBuffer[1]);

	drawLine(VBuffer[5], CBuffer[5], VBuffer[1], CBuffer[1]);
	drawLine(VBuffer[5], CBuffer[5], VBuffer[2], CBuffer[2]);
	drawLine(VBuffer[5], CBuffer[5], VBuffer[3], CBuffer[3]);
	drawLine(VBuffer[5], CBuffer[5], VBuffer[4], CBuffer[4]);

}

// --------------------------------------------------------------------------



void RenderAux::drawFrustum(const XFrustum& frustum, const Color8u& nearCol, const Color8u& farCol,
	bool drawShaded)
{
	std::array<Vec3f, 8> v;
	frustum.GetFrustumVertices(v);

	if (drawShaded)
	{
		// draw the 3 planes as shaded solids.
		XAuxVertex* pVertices = nullptr;
		uint16* pIndices = nullptr;
		addIndexedPrimitive(pVertices, 8, pIndices, (6 * 6), createTriangleRenderFlags(true));

		// far
		pVertices[0].pos = v[0];
		pVertices[0].color = farCol;
		pVertices[1].pos = v[1];
		pVertices[1].color = farCol;
		pVertices[2].pos = v[2];
		pVertices[2].color = farCol;
		pVertices[3].pos = v[3];
		pVertices[3].color = farCol;

		pIndices[0] = 0; pIndices[1] = 1; pIndices[2] = 2;
		pIndices[3] = 0; pIndices[4] = 2; pIndices[5] = 3;

		// near
		pVertices[4].pos = v[4];
		pVertices[4].color = nearCol;
		pVertices[5].pos = v[5];
		pVertices[5].color = nearCol;
		pVertices[6].pos = v[6];
		pVertices[6].color = nearCol;
		pVertices[7].pos = v[7];
		pVertices[7].color = nearCol;

		pIndices[6] = 0 + 4; pIndices[7] = 1 + 4; pIndices[8] = 2 + 4;
		pIndices[9] = 0 + 4; pIndices[10] = 2 + 4; pIndices[11] = 3 + 4;

		// shaded sisdes.

		// 0 ------ 3
		// |		|
		// |		|
		// 1 ------ 2

		// side1
		pIndices[12] = 0;
		pIndices[13] = 1;
		pIndices[14] = 1 + 4;
		pIndices[15] = 0;
		pIndices[16] = 0 + 4;
		pIndices[17] = 1 + 4;


		// side2
		pIndices[18] = 0;
		pIndices[19] = 0 + 4;
		pIndices[20] = 3 + 4;
		pIndices[21] = 0;
		pIndices[22] = 3;
		pIndices[23] = 3 + 4;


		// side3
		pIndices[24] = 3;
		pIndices[25] = 2;
		pIndices[26] = 3 + 4;
		pIndices[27] = 3 + 4;
		pIndices[28] = 2 + 4;
		pIndices[29] = 2;

		// side4
		// 1 2
		// 2
		pIndices[30] = 2;
		pIndices[31] = 1 + 4;
		pIndices[32] = 2 + 4;

		// 1
		// 1 2
		pIndices[33] = 1 + 4;
		pIndices[34] = 1;
		pIndices[35] = 2;

		Color8u lineColFar(farCol);
		Color8u lineColNear(nearCol);

		lineColFar.a = 255;
		lineColNear.a = 255;

		// connect them with lines.
		for (size_t i = 0; i < 4; i++)
		{
			drawLine(v[i], lineColFar, v[((i + 1) & 3)], lineColFar);
			drawLine(v[4 + i], lineColNear, v[4 + ((i + 1) & 3)], lineColNear);

			// far to near
			drawLine(v[i], lineColFar, v[4 + i], lineColNear);
		}
	}
	else
	{
		for (size_t i = 0; i < 4; i++)
		{
			drawLine(v[i], farCol, v[((i + 1) & 3)], farCol);
			drawLine(v[4 + i], nearCol, v[4 + ((i + 1) & 3)], nearCol);

			// far to near
			drawLine(v[i], farCol, v[4 + i], nearCol);
		}
	}
}

// --------------------------------------------------------------------------

void RenderAux::drawArrow(const Vec3f& posA, const Vec3f& posB, const Color8u& color)
{
	const Vec3f t0 = (posB - posA).normalized();
	const Vec3f a = math<float32_t>::abs(t0.x) < 0.707f ? Vec3f(1, 0, 0) : Vec3f(0, 1, 0);
	const Vec3f t1 = t0.cross(a).normalized();
	const Vec3f t2 = t0.cross(t1).normalized();

	Vec3f points[10] = {
		posA, posB,
		posB, posB - t0 * 0.15f + t1 * 0.15f,
		posB, posB - t0 * 0.15f - t1 * 0.15f,
		posB, posB - t0 * 0.15f + t2 * 0.15f,
		posB, posB - t0 * 0.15f - t2 * 0.15f
	};

	drawLines(points, 10, color);
}


// --------------------------------------------------------------------------



void RenderAux::addPushBufferEntry(uint32 numVertices, uint32 numIndices,
	const XAuxGeomRenderFlags& renderFlags)
{
	AuxPushArr& auxPushArr = data_.auxPushArr;
	PrimType::Enum primType = getPrimType(renderFlags);

	if (auxPushArr.isNotEmpty() &&
		auxPushArr[auxPushArr.size() - 1].renderFlags == renderFlags &&
		(PrimType::PointList == primType || PrimType::LineList == primType || PrimType::TriList == primType))
	{
		// Perform a runtime optimization (pre-merging) which effectively reduces the number of PB entries created.
		// We can merge this entry with the previous one as its render flags match with the ones of the previous entry
		// (e.g. merges consecutive DrawLine(...) calls with same render flags into one PB entry).

		// Only done for non-indexed primitives as otherwise there would be the additional overhead of patching
		// the indices for each push buffer entry. Indices already and still have to be patched during rendering 
		// anyway (merging) so in case of indexed primitives there'd be no real benefit. Also, merging up too many 
		// indexed primitves could potentially produce a push buffer entry which cannot be rendered as it exceeds 
		// the vb/ib buffer size for auxiliary geometries in the renderer.
		AuxPushBufferEntry& lastPBEntry = auxPushArr[auxPushArr.size() - 1];
		lastPBEntry.numVertices += numVertices;
		lastPBEntry.numIndices += numIndices;
	}
	else
	{
		// create new push buffer entry
		auxPushArr.emplace_back(numVertices, numIndices,
			safe_static_cast<uint32_t, size_t>(data_.auxVertexArr.size()),
			safe_static_cast<uint32_t, size_t>(data_.auxIndexArr.size()),
			renderFlags
		);
	}
}


void RenderAux::addPrimitive(XAuxVertex*& pVertices, uint32 numVertices,
	const XAuxGeomRenderFlags& renderFlags)
{
	// add push buffer entry to allow later merging of batches commited via DP
	addPushBufferEntry(numVertices, 0, renderFlags);

	// get vertex ptr
	AuxVertexArr& auxVertexArr = data_.auxVertexArr;
	AuxVertexArr::size_type oldVBSize = auxVertexArr.size();
	auxVertexArr.resize(oldVBSize + numVertices);
	pVertices = &auxVertexArr[oldVBSize];
}


void RenderAux::addIndexedPrimitive(XAuxVertex*& pVertices, uint32 numVertices,
	uint16*& pIndices, uint32 numIndices, const XAuxGeomRenderFlags& renderFlags)
{
	// add push buffer entry to allow later merging of batches commited via DIP
	addPushBufferEntry(numVertices, numIndices, renderFlags);

	// get vertex ptr
	AuxVertexArr& auxVertexArr = data_.auxVertexArr;
	AuxVertexArr::size_type oldVBSize = auxVertexArr.size();
	auxVertexArr.resize(oldVBSize + numVertices);
	pVertices = &auxVertexArr[oldVBSize];

	// get index ptr
	AuxIndexArr& auxIndexArr = data_.auxIndexArr;
	AuxIndexArr::size_type oldIBSize = auxIndexArr.size();
	auxIndexArr.resize(oldIBSize + numIndices);
	pIndices = &auxIndexArr[oldIBSize];
}


void RenderAux::addObject(AuxDrawObjParams*& pDrawParams,
	const XAuxGeomRenderFlags& renderFlags)
{
	// create new push buffer entry
	AuxPushArr& auxPushArr = data_.auxPushArr;
	AuxDrawObjParamArr& auxDrawObjParamArr = data_.auxDrawObjParamArr;

	auxPushArr.emplace_back(
		safe_static_cast<uint32_t, size_t>(auxDrawObjParamArr.size()),
		renderFlags
	);

	// get draw param buffer ptr
	AuxDrawObjParamArr::size_type oldSize = auxDrawObjParamArr.size();
	auxDrawObjParamArr.resize(oldSize + 1);
	pDrawParams = &auxDrawObjParamArr[oldSize];
}

X_NAMESPACE_END