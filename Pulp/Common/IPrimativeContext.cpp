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

	PrimVertex* pQuad = addPrimative(6, PrimitiveType::TRIANGLELIST, pMaterial);

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

	// BL
	pQuad[4] = pQuad[2];
	// TR
	pQuad[5] = pQuad[1];
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

void IPrimativeContext::drawLines(const Vec3f* pPoints, uint32_t num, const Color8u& col)
{
	X_ASSERT_NOT_NULL(pPoints);
	X_ASSERT((num % 2) == 0, "num points must be a multiple of 2")(num);

	if (num < 2) { // 2 points needed to make a line.
		return;
	}

	PrimVertex* pLine = addPrimative(num, PrimitiveType::LINELIST);

	for (uint32_t i = 0; i < num; i += 2)
	{
		pLine[i].pos = pPoints[i];
		pLine[i].color = col;
		pLine[i].st = core::XHalf2::zero();
		pLine[i + 1].pos = pPoints[i + 1];
		pLine[i + 1].color = col;
		pLine[i + 1].st = core::XHalf2::zero();
	}
}


void IPrimativeContext::drawRect(float x, float y, float width, float height, const Color8u& col)
{
	const float x1 = x;
	const float y1 = y;
	const float x2 = x + width;
	const float y2 = y + height;

	const Vec3f tl(x1, y1, 0);
	const Vec3f tr(x2, y1, 0);
	const Vec3f bl(x1, y2, 0);
	const Vec3f br(x2, y2, 0);

	drawRect(tl, tr, bl, br, col);
}

void IPrimativeContext::drawRect(const Vec3f& tl, const Vec3f& tr, const Vec3f& bl, const Vec3f& br, const Color8u& col)
{
	const Vec3f points[8] = {
		tl, tr,
		bl, br,
		tl, bl,
		tr, br
	};

	drawLines(points, X_ARRAY_SIZE(points), col);
}

void IPrimativeContext::drawBarChart(const Rectf& rect, uint32_t num, const float* pHeights,
	float padding, uint32_t max, const Color8u& col)
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

	PrimVertex* pQuads = addPrimative(num * 6, PrimitiveType::TRIANGLELIST);

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

void IPrimativeContext::drawAABB(const AABB& aabb, bool solid, const Color8u& col)
{
	// for now we do this none indexed
	if (!solid)
	{
		const uint32_t numPoints = 24;

		// so we need 4 * 3 lines.
		PrimVertex* pLines = addPrimative(numPoints, PrimitiveType::LINELIST);

		// create the 8 points.
		const Vec3f& xyz = aabb.min;
		const Vec3f xyZ(aabb.min.x, aabb.min.y, aabb.max.z);
		const Vec3f xYz(aabb.min.x, aabb.max.y, aabb.min.z);
		const Vec3f xYZ(aabb.min.x, aabb.max.y, aabb.max.z);
		const Vec3f Xyz(aabb.max.x, aabb.min.y, aabb.min.z);
		const Vec3f XyZ(aabb.max.x, aabb.min.y, aabb.max.z);
		const Vec3f XYz(aabb.max.x, aabb.max.y, aabb.min.z);
		const Vec3f& XYZ = aabb.max;

		// bottom row (all lower case z's)
		pLines[0].pos = xyz;
		pLines[1].pos = xYz;
		
		pLines[2].pos = xYz;
		pLines[3].pos = XYz;

		pLines[4].pos = XYz;
		pLines[5].pos = Xyz;

		pLines[6].pos = Xyz;
		pLines[7].pos = xyz;

		// middle (all lower case z's pointing to upper Z's :D)
		pLines[8].pos = xyz;
		pLines[9].pos = xyZ;

		pLines[10].pos = xYz;
		pLines[11].pos = xYZ;

		pLines[12].pos = XYz;
		pLines[13].pos = XYZ;

		pLines[14].pos = Xyz;
		pLines[15].pos = XyZ;

		// top row (all upper case z's)
		pLines[16].pos = xyZ;
		pLines[17].pos = xYZ;

		pLines[18].pos = xYZ;
		pLines[19].pos = XYZ;

		pLines[20].pos = XYZ;
		pLines[21].pos = XyZ;

		pLines[22].pos = XyZ;
		pLines[23].pos = xyZ;

		// colors, maybe mix this into pos assignment, since all these verts don't fit in single cache lane ;(
		for (uint32_t i = 0; i < numPoints; i++)
		{
			pLines[i].color = col;
		}
	}
	else
	{
		// we do this as triangles.
		PrimVertex* pVerts = addPrimative(36, PrimitiveType::TRIANGLELIST);

		const Vec3f& xyz = aabb.min;
		const Vec3f xyZ(aabb.min.x, aabb.min.y, aabb.max.z);
		const Vec3f xYz(aabb.min.x, aabb.max.y, aabb.min.z);
		const Vec3f xYZ(aabb.min.x, aabb.max.y, aabb.max.z);
		const Vec3f Xyz(aabb.max.x, aabb.min.y, aabb.min.z);
		const Vec3f XyZ(aabb.max.x, aabb.min.y, aabb.max.z);
		const Vec3f XYz(aabb.max.x, aabb.max.y, aabb.min.z);
		const Vec3f& XYZ = aabb.max;

		Color8u colBot(col * 0.5f);
		Color8u colTop(col);
		Color8u colBack(col * 0.5f);
		Color8u colFront(col * 0.9f);
		Color8u colLeft(col * 0.7f);
		Color8u colRight(col * 0.8f);

		// bottom
		pVerts[0].pos = xyz;
		pVerts[0].color = colBot;
		pVerts[1].pos = xYz;
		pVerts[1].color = colBot;
		pVerts[2].pos = XYz;
		pVerts[2].color = colBot;

		pVerts[3].pos = xyz;
		pVerts[3].color = colBot;
		pVerts[4].pos = XYz;
		pVerts[4].color = colBot;
		pVerts[5].pos = Xyz;
		pVerts[5].color = colBot;

		// top
		pVerts[6].pos = xyZ;
		pVerts[6].color = colTop;
		pVerts[7].pos = xYZ;
		pVerts[7].color = colTop;
		pVerts[8].pos = XYZ;
		pVerts[8].color = colTop;

		pVerts[9].pos = xyZ;
		pVerts[9].color = colTop;
		pVerts[10].pos = XYZ;
		pVerts[10].color = colTop;
		pVerts[11].pos = XyZ;
		pVerts[11].color = colTop;

		// back.
		pVerts[12].pos = xyz;
		pVerts[12].color = colBack;
		pVerts[13].pos = Xyz;
		pVerts[13].color = colBack;
		pVerts[14].pos = XyZ;
		pVerts[14].color = colBack;

		pVerts[15].pos = xyz;
		pVerts[15].color = colBack;
		pVerts[16].pos = XyZ;
		pVerts[16].color = colBack;
		pVerts[17].pos = xyZ;
		pVerts[17].color = colBack;

		// front
		pVerts[18].pos = xYz;
		pVerts[18].color = colFront;
		pVerts[19].pos = xYZ;
		pVerts[19].color = colFront;
		pVerts[20].pos = XYZ;
		pVerts[20].color = colFront;

		pVerts[21].pos = xYz;
		pVerts[21].color = colFront;
		pVerts[22].pos = XYZ;
		pVerts[22].color = colFront;
		pVerts[23].pos = XYz;
		pVerts[23].color = colFront;

		// left
		pVerts[24].pos = xyz;
		pVerts[24].color = colLeft;
		pVerts[25].pos = xyZ;
		pVerts[25].color = colLeft;
		pVerts[26].pos = xYZ;
		pVerts[26].color = colLeft;

		pVerts[27].pos = xyz;
		pVerts[27].color = colLeft;
		pVerts[28].pos = xYZ;
		pVerts[28].color = colLeft;
		pVerts[29].pos = xYz;
		pVerts[29].color = colLeft;

		// right
		pVerts[30].pos = Xyz;
		pVerts[30].color = colRight;
		pVerts[31].pos = XYz;
		pVerts[31].color = colRight;
		pVerts[32].pos = XYZ;
		pVerts[32].color = colRight;

		pVerts[33].pos = Xyz;
		pVerts[33].color = colRight;
		pVerts[34].pos = XYZ;
		pVerts[34].color = colRight;
		pVerts[35].pos = XyZ;
		pVerts[35].color = colRight;

	}
}

// Sphere
void IPrimativeContext::drawSphere(const Sphere& sphere, const Color8u& col, bool solid, int32_t lodIdx)
{
	// fuck a goat with a flag pole.
	// this really needs to be implemented with premade sphere mesh that is just instanced.
	// we can also get fancy and do lods based on distance.
	// in order todo instance the prim contex should create the render shapes itself and handle the instancing.
	// so i will make a seperate api for adding these, that deals with specific shapes.
	// the prim contex impl will handle the instanced drawing.

	if (sphere.radius() > 0.0f)
	{
		Matrix44f mat = Matrix44f::createScale(Vec3f(sphere.radius()));
		mat.setTranslate(sphere.center());

		ShapeInstanceData* pObj = addShape(ShapeType::Sphere, solid, lodIdx);
		pObj->mat = mat;
		pObj->color = col;
	}
}

void IPrimativeContext::drawSphere(const Sphere& sphere, const Matrix34f& mat, const Color8u& col, bool solid, int32_t lodIdx)
{
	if (sphere.radius() > 0.0f)
	{
		Matrix44f scale = Matrix44f::createScale(sphere.radius());
		Matrix44f trans = Matrix44f::createTranslation(mat * sphere.center());
		Matrix44f transMat = trans * scale;

		ShapeInstanceData* pObj = addShape(ShapeType::Sphere, solid, lodIdx);
		pObj->mat = transMat;
		pObj->color = col;
	}
}

// Cone
void IPrimativeContext::drawCone(const Vec3f& pos, const Vec3f& dir, float radius, float height, 
	const Color8u& col, bool solid, int32_t lodIdx)
{
	if (radius > 0.0f && height > 0.0f && dir.lengthSquared() > 0.0f)
	{
		Vec3f direction(dir.normalized());
		Vec3f orthogonal(direction.getOrthogonal().normalized());

		Matrix44f matRot;
		matRot.setToIdentity();
		matRot.setColumn(0, orthogonal);
		matRot.setColumn(1, direction);
		matRot.setColumn(2, orthogonal.cross(direction));

		Matrix44f scale = Matrix44f::createScale(Vec3f(radius, height, radius));
		Matrix44f trans = Matrix44f::createTranslation(pos);
		Matrix44f transMat = trans * matRot * scale;

		ShapeInstanceData* pObj = addShape(ShapeType::Cone, solid, lodIdx);
		pObj->mat = transMat;
		pObj->color = col;
	}
}

// Cylinder
void IPrimativeContext::drawCylinder(const Vec3f& pos, const Vec3f& dir, float radius, float height, 
	const Color8u& col, bool solid, int32_t lodIdx)
{
	if (radius > 0.0f && height > 0.0f && dir.lengthSquared() > 0.0f)
	{
		Vec3f direction(dir.normalized());
		Vec3f orthogonal(direction.getOrthogonal().normalized());

		Matrix44f matRot;
		matRot.setToIdentity();
		matRot.setColumn(0, orthogonal);
		matRot.setColumn(1, direction);
		matRot.setColumn(2, orthogonal.cross(direction));

		Matrix44f scale = Matrix44f::createScale(Vec3f(radius, height, radius));
		Matrix44f trans = Matrix44f::createTranslation(pos);
		Matrix44f transMat = trans * matRot * scale;

		ShapeInstanceData* pObj = addShape(ShapeType::Cylinder, solid, lodIdx);
		pObj->mat = transMat;
		pObj->color = col;
	}
}

// Bone
void IPrimativeContext::drawBone(const Transformf& rParent, const Transformf& rChild, const Color8u& col)
{
	Vec3f p = rParent.getPosition();
	Vec3f c = rChild.getPosition();
	Vec3f vBoneVec = c - p;
	float fBoneLength = vBoneVec.length();

	if (fBoneLength < 1e-4) {
		return;
	}

	Matrix33f m33 = Matrix33f::createRotationV01(Vec3f(1, 0, 0), vBoneVec / fBoneLength);
	Matrix34f m34 = Matrix34f(m33, p);

	float32_t t = fBoneLength*0.025f;

	//bone points in x-direction
	const Vec3f s = Vec3f::zero();
	const Vec3f m0 = Vec3f(t, +t, +t);
	const Vec3f m1 = Vec3f(t, -t, +t);
	const Vec3f m2 = Vec3f(t, -t, -t);
	const Vec3f m3 = Vec3f(t, +t, -t);
	const Vec3f e = Vec3f(fBoneLength, 0, 0);

	// wut. inverse color i think?
	Color8u comp;
	comp.r = col.r ^ 0xFF;
	comp.r = (col.r - comp.r) / 2;
	comp.g = col.g ^ 0xFF;
	comp.g = (col.g - comp.g) / 2;
	comp.b = col.b ^ 0xFF;
	comp.b = (col.b - comp.b) / 2;
	comp.a = col.a;

	Vec3f points[6] = {
		m34*s,
		m34*m0,
		m34*m1,
		m34*m2,
		m34*m3,
		m34*e
	};

	// we draw some lines. :)

	PrimVertex* pLines = addPrimative(24, PrimitiveType::LINELIST);
	pLines[0].pos = points[0];
	pLines[0].color = comp;
	pLines[1].pos = points[1];
	pLines[1].color = col;
	pLines[2].pos = points[0];
	pLines[2].color = comp;
	pLines[3].pos = points[2];
	pLines[3].color = col;
	pLines[4].pos = points[0];
	pLines[4].color = comp;
	pLines[5].pos = points[3];
	pLines[5].color = col;
	pLines[6].pos = points[0];
	pLines[6].color = comp;
	pLines[7].pos = points[4];
	pLines[7].color = col;

	// middle
	pLines[8].pos = points[1];
	pLines[8].color = col;
	pLines[9].pos = points[2];
	pLines[9].color = col;
	pLines[10].pos = points[2];
	pLines[10].color = col;
	pLines[11].pos = points[3];
	pLines[11].color = col;
	pLines[12].pos = points[3];
	pLines[12].color = col;
	pLines[13].pos = points[4];
	pLines[13].color = col;
	pLines[14].pos = points[4];
	pLines[14].color = col;
	pLines[15].pos = points[1];
	pLines[15].color = col;

	// top :D !!
	pLines[16].pos = points[5];
	pLines[16].color = comp;
	pLines[17].pos = points[1];
	pLines[17].color = col;
	pLines[18].pos = points[5];
	pLines[18].color = comp;
	pLines[19].pos = points[2];
	pLines[19].color = col;
	pLines[20].pos = points[5];
	pLines[20].color = comp;
	pLines[21].pos = points[3];
	pLines[21].color = col;
	pLines[22].pos = points[5];
	pLines[22].color = comp;
	pLines[23].pos = points[4];
	pLines[23].color = col;

}


// Frustum - Sexyyyyyyyyyy
void IPrimativeContext::drawFrustum(const XFrustum& frustum, const Color8u& nearCol, const Color8u& farCol, bool solid)
{
	std::array<Vec3f, 8> v;
	frustum.GetFrustumVertices(v);

	if (!solid)
	{
		PrimVertex* pLines = addPrimative(4 * 6, PrimitiveType::LINELIST);

		for (size_t i = 0; i < 4; i++)
		{
			pLines[0].pos = v[i];
			pLines[0].color = farCol;
			pLines[1].pos = v[((i + 1) & 3)];
			pLines[1].color = farCol;

			pLines[2].pos = v[i + 4];
			pLines[2].color = nearCol;
			pLines[3].pos = v[((i + 1) & 3) + 4];
			pLines[3].color = nearCol;

			pLines[4].pos = v[i];
			pLines[4].color = farCol;
			pLines[5].pos = v[i + 4];
			pLines[5].color = nearCol;

			pLines += 6;
		}
	}
	else
	{
		PrimVertex* pVertices = addPrimative(6 * 2, PrimitiveType::TRIANGLELIST);

		// far
		pVertices[0].pos = v[0];
		pVertices[0].color = farCol;
		pVertices[1].pos = v[1];
		pVertices[1].color = farCol;
		pVertices[2].pos = v[2];
		pVertices[2].color = farCol;

		pVertices[3].pos = v[0];
		pVertices[3].color = farCol;
		pVertices[4].pos = v[2];
		pVertices[4].color = farCol;
		pVertices[5].pos = v[3];
		pVertices[5].color = farCol;

		// near
		pVertices[6].pos = v[4];
		pVertices[6].color = nearCol;
		pVertices[7].pos = v[5];
		pVertices[7].color = nearCol;
		pVertices[8].pos = v[6];
		pVertices[8].color = nearCol;

		pVertices[9].pos = v[4];
		pVertices[9].color = nearCol;
		pVertices[10].pos = v[6];
		pVertices[10].color = nearCol;
		pVertices[11].pos = v[7];
		pVertices[11].color = nearCol;

		PrimVertex* pLines = addPrimative(4 * 6, PrimitiveType::LINELIST);

		Color8u lineColFar(farCol);
		Color8u lineColNear(nearCol);

		lineColFar.a = 255;
		lineColNear.a = 255;

		for (size_t i = 0; i < 4; i++)
		{
			pLines[0].pos = v[i];
			pLines[0].color = lineColFar;
			pLines[1].pos = v[((i + 1) & 3)];
			pLines[1].color = lineColFar;

			pLines[2].pos = v[i + 4];
			pLines[2].color = lineColNear;
			pLines[3].pos = v[((i + 1) & 3) + 4];
			pLines[3].color = lineColNear;

			pLines[4].pos = v[i];
			pLines[4].color = lineColFar;
			pLines[5].pos = v[i + 4];
			pLines[5].color = lineColNear;

			pLines += 6;
		}
	}
}

// Arrow
void IPrimativeContext::drawArrow(const Vec3f& posA, const Vec3f& posB, const Color8u& color)
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

	drawLines(points, X_ARRAY_SIZE(points), color);
}

void IPrimativeContext::drawCrosshair(const Vec3f& pos, size_t size, const Color8u& color)
{
	Vec3f points[4] = {
		Vec3f(pos.x - size, pos.y, pos.z), // left
		Vec3f(pos.x + size, pos.y, pos.z), // right
		Vec3f(pos.x, pos.y - size, pos.z), // top
		Vec3f(pos.x, pos.y + size, pos.z)  // bottom
	};

	drawLines(points, X_ARRAY_SIZE(points), color);
}



void IPrimativeContext::drawImageWithUV(float xpos, float ypos, float z, float w, float h,
	Material* pMaterial, const float* s, const float* t,
	const Colorf& col, bool filtered)
{
	X_ASSERT_NOT_NULL(s);
	X_ASSERT_NOT_NULL(t);
	X_UNUSED(filtered);

	PrimVertex* pQuad = addPrimative(6, PrimitiveType::TRIANGLELIST, pMaterial);

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

	for (uint32 i = 0; i<6; ++i)
	{
		pQuad[i].color = col;
		pQuad[i].st = core::XHalf2::compress(s[i], t[i]);
	}

	// BL
	pQuad[4] = pQuad[2];
	// TR
	pQuad[5] = pQuad[1];
}


X_NAMESPACE_END