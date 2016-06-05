#include "stdafx.h"
#include "DebugRender.h"

X_NAMESPACE_BEGIN(physics)

namespace
{

	X_INLINE Vec3f Vec3FromPhysx(const physx::PxVec3& vec)
	{
		return Vec3f(vec.x, vec.y, vec.z);
	}

	static const Vec3f gCapsuleVertices[] =
	{
		Vec3f(0.0000f, -2.0000f, -0.0000f),
		Vec3f(0.3827f, -1.9239f, -0.0000f),
		Vec3f(0.2706f, -1.9239f, 0.2706f),
		Vec3f(-0.0000f, -1.9239f, 0.3827f),
		Vec3f(-0.2706f, -1.9239f, 0.2706f),
		Vec3f(-0.3827f, -1.9239f, -0.0000f),
		Vec3f(-0.2706f, -1.9239f, -0.2706f),
		Vec3f(0.0000f, -1.9239f, -0.3827f),
		Vec3f(0.2706f, -1.9239f, -0.2706f),
		Vec3f(0.7071f, -1.7071f, -0.0000f),
		Vec3f(0.5000f, -1.7071f, 0.5000f),
		Vec3f(-0.0000f, -1.7071f, 0.7071f),
		Vec3f(-0.5000f, -1.7071f, 0.5000f),
		Vec3f(-0.7071f, -1.7071f, -0.0000f),
		Vec3f(-0.5000f, -1.7071f, -0.5000f),
		Vec3f(0.0000f, -1.7071f, -0.7071f),
		Vec3f(0.5000f, -1.7071f, -0.5000f),
		Vec3f(0.9239f, -1.3827f, -0.0000f),
		Vec3f(0.6533f, -1.3827f, 0.6533f),
		Vec3f(-0.0000f, -1.3827f, 0.9239f),
		Vec3f(-0.6533f, -1.3827f, 0.6533f),
		Vec3f(-0.9239f, -1.3827f, -0.0000f),
		Vec3f(-0.6533f, -1.3827f, -0.6533f),
		Vec3f(0.0000f, -1.3827f, -0.9239f),
		Vec3f(0.6533f, -1.3827f, -0.6533f),
		Vec3f(1.0000f, -1.0000f, -0.0000f),
		Vec3f(0.7071f, -1.0000f, 0.7071f),
		Vec3f(-0.0000f, -1.0000f, 1.0000f),
		Vec3f(-0.7071f, -1.0000f, 0.7071f),
		Vec3f(-1.0000f, -1.0000f, -0.0000f),
		Vec3f(-0.7071f, -1.0000f, -0.7071f),
		Vec3f(0.0000f, -1.0000f, -1.0000f),
		Vec3f(0.7071f, -1.0000f, -0.7071f),
		Vec3f(1.0000f, 1.0000f, 0.0000f),
		Vec3f(0.7071f, 1.0000f, 0.7071f),
		Vec3f(-0.0000f, 1.0000f, 1.0000f),
		Vec3f(-0.7071f, 1.0000f, 0.7071f),
		Vec3f(-1.0000f, 1.0000f, -0.0000f),
		Vec3f(-0.7071f, 1.0000f, -0.7071f),
		Vec3f(0.0000f, 1.0000f, -1.0000f),
		Vec3f(0.7071f, 1.0000f, -0.7071f),
		Vec3f(0.9239f, 1.3827f, 0.0000f),
		Vec3f(0.6533f, 1.3827f, 0.6533f),
		Vec3f(-0.0000f, 1.3827f, 0.9239f),
		Vec3f(-0.6533f, 1.3827f, 0.6533f),
		Vec3f(-0.9239f, 1.3827f, -0.0000f),
		Vec3f(-0.6533f, 1.3827f, -0.6533f),
		Vec3f(0.0000f, 1.3827f, -0.9239f),
		Vec3f(0.6533f, 1.3827f, -0.6533f),
		Vec3f(0.7071f, 1.7071f, 0.0000f),
		Vec3f(0.5000f, 1.7071f, 0.5000f),
		Vec3f(-0.0000f, 1.7071f, 0.7071f),
		Vec3f(-0.5000f, 1.7071f, 0.5000f),
		Vec3f(-0.7071f, 1.7071f, 0.0000f),
		Vec3f(-0.5000f, 1.7071f, -0.5000f),
		Vec3f(0.0000f, 1.7071f, -0.7071f),
		Vec3f(0.5000f, 1.7071f, -0.5000f),
		Vec3f(0.3827f, 1.9239f, 0.0000f),
		Vec3f(0.2706f, 1.9239f, 0.2706f),
		Vec3f(-0.0000f, 1.9239f, 0.3827f),
		Vec3f(-0.2706f, 1.9239f, 0.2706f),
		Vec3f(-0.3827f, 1.9239f, 0.0000f),
		Vec3f(-0.2706f, 1.9239f, -0.2706f),
		Vec3f(0.0000f, 1.9239f, -0.3827f),
		Vec3f(0.2706f, 1.9239f, -0.2706f),
		Vec3f(0.0000f, 2.0000f, 0.0000f),
	};

	static const uint8_t gCapsuleIndices[] =
	{
		1, 0, 2,  2, 0, 3,  3, 0, 4,  4, 0, 5,  5, 0, 6,  6, 0, 7,  7, 0, 8,
		8, 0, 1,  9, 1, 10,  10, 1, 2,  10, 2, 11,  11, 2, 3,  11, 3, 12,
		12, 3, 4,  12, 4, 13,  13, 4, 5,  13, 5, 14,  14, 5, 6,  14, 6, 15,
		15, 6, 7,  15, 7, 16,  16, 7, 8,  16, 8, 9,  9, 8, 1,  17, 9, 18,
		18, 9, 10,  18, 10, 19,  19, 10, 11,  19, 11, 20,  20, 11, 12,  20, 12, 21,
		21, 12, 13,  21, 13, 22,  22, 13, 14,  22, 14, 23,  23, 14, 15,  23, 15, 24,
		24, 15, 16,  24, 16, 17,  17, 16, 9,  25, 17, 26,  26, 17, 18,  26, 18, 27,
		27, 18, 19,  27, 19, 28,  28, 19, 20,  28, 20, 29,  29, 20, 21,  29, 21, 30,
		30, 21, 22,  30, 22, 31,  31, 22, 23,  31, 23, 32,  32, 23, 24,  32, 24, 25,
		25, 24, 17,  33, 25, 34,  34, 25, 26,  34, 26, 35,  35, 26, 27,  35, 27, 36,
		36, 27, 28,  36, 28, 37,  37, 28, 29,  37, 29, 38,  38, 29, 30,  38, 30, 39,
		39, 30, 31,  39, 31, 40,  40, 31, 32,  40, 32, 33,  33, 32, 25,  41, 33, 42,
		42, 33, 34,  42, 34, 43,  43, 34, 35,  43, 35, 44,  44, 35, 36,  44, 36, 45,
		45, 36, 37,  45, 37, 46,  46, 37, 38,  46, 38, 47,  47, 38, 39,  47, 39, 48,
		48, 39, 40,  48, 40, 41,  41, 40, 33,  49, 41, 50,  50, 41, 42,  50, 42, 51,
		51, 42, 43,  51, 43, 52,  52, 43, 44,  52, 44, 53,  53, 44, 45,  53, 45, 54,
		54, 45, 46,  54, 46, 55,  55, 46, 47,  55, 47, 56,  56, 47, 48,  56, 48, 49,
		49, 48, 41,  57, 49, 58,  58, 49, 50,  58, 50, 59,  59, 50, 51,  59, 51, 60,
		60, 51, 52,  60, 52, 61,  61, 52, 53,  61, 53, 62,  62, 53, 54,  62, 54, 63,
		63, 54, 55,  63, 55, 64,  64, 55, 56,  64, 56, 57,  57, 56, 49,  65, 57, 58,
		65, 58, 59,  65, 59, 60,  65, 60, 61,  65, 61, 62,  65, 62, 63,  65, 63, 64,
		65, 64, 57,
	};

	static const size_t gNumCapsuleIndices = ARRAYSIZE(gCapsuleIndices);

} // namespace


DebugRender::DebugRender()
{

}

DebugRender::~DebugRender()
{

}

void DebugRender::update(const physx::PxRenderBuffer& debugRenderable)
{
	// Points
	const uint32_t numPoints = debugRenderable.getNbPoints();
	if (numPoints)
	{
		const physx::PxDebugPoint* PX_RESTRICT points = debugRenderable.getPoints();
		checkResizePoint(numPoints);

		for (uint32_t i = 0; i<numPoints; i++)
		{
			const physx::PxDebugPoint& point = points[i];
			addPoint(Vec3FromPhysx(point.pos), Color8u::hexA(point.color));
		}
	}

	// Lines
	const uint32_t numLines = debugRenderable.getNbLines();
	if (numLines)
	{
		const physx::PxDebugLine* PX_RESTRICT lines = debugRenderable.getLines();
		checkResizeLine(numLines * 2);

		for (uint32_t i = 0; i<numLines; i++)
		{
			const physx::PxDebugLine& line = lines[i];
			addLine(Vec3FromPhysx(line.pos0), Vec3FromPhysx(line.pos1), Color8u::hexA(line.color0));
		}
	}

	// Triangles
	const uint32_t numTriangles = debugRenderable.getNbTriangles();
	if (numTriangles)
	{
		const physx::PxDebugTriangle* PX_RESTRICT triangles = debugRenderable.getTriangles();
		checkResizeTriangle(numTriangles * 3);

		for (uint32_t i = 0; i<numTriangles; i++)
		{
			const physx::PxDebugTriangle& triangle = triangles[i];
			addTriangle(Vec3FromPhysx(triangle.pos0),
				Vec3FromPhysx(triangle.pos1), 
				Vec3FromPhysx(triangle.pos2), 
				Color8u::hexA(triangle.color0));
		}
	}
}

void DebugRender::queueForRender()
{
	queueForRenderPoint();
	queueForRenderLine();
	queueForRenderTriangle();
}

void DebugRender::clear()
{
	clearPoint();
	clearLine();
	clearTriangle();
}

void DebugRender::addAABB(const physx::PxBounds3& box, const Color8u& color, DrawFlags renderFlags)
{
	const physx::PxVec3& min = box.minimum;
	const physx::PxVec3& max = box.maximum;

	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	// Generate 8 corners of the bbox
	Vec3f pts[8];
	pts[0] = Vec3f(min.x, min.y, min.z);
	pts[1] = Vec3f(max.x, min.y, min.z);
	pts[2] = Vec3f(max.x, max.y, min.z);
	pts[3] = Vec3f(min.x, max.y, min.z);
	pts[4] = Vec3f(min.x, min.y, max.z);
	pts[5] = Vec3f(max.x, min.y, max.z);
	pts[6] = Vec3f(max.x, max.y, max.z);
	pts[7] = Vec3f(min.x, max.y, max.z);

	addBox(pts, color, renderFlags);
}

void DebugRender::addOBB(const physx::PxVec3& boxCenter, const physx::PxVec3& boxExtents, 
	const physx::PxMat33& boxRot, const Color8u& color, DrawFlags renderFlags)
{
	Vec3f Axis0 = Vec3FromPhysx(boxRot.column0);
	Vec3f Axis1 = Vec3FromPhysx(boxRot.column1);
	Vec3f Axis2 = Vec3FromPhysx(boxRot.column2);

	// "Rotated extents"
	Axis0 *= boxExtents.x;
	Axis1 *= boxExtents.y;
	Axis2 *= boxExtents.z;

	//     7+------+6			0 = ---
	//     /|     /|			1 = +--
	//    / |    / |			2 = ++-
	//   / 4+---/--+5			3 = -+-
	// 3+------+2 /    y   z	4 = --+
	//  | /    | /     |  /		5 = +-+
	//  |/     |/      |/		6 = +++
	// 0+------+1      *---x	7 = -++

	Vec3f pts[8];
	pts[0] = pts[3] = pts[4] = pts[7] = Vec3FromPhysx(boxCenter) - Axis0;
	pts[1] = pts[2] = pts[5] = pts[6] = Vec3FromPhysx(boxCenter) + Axis0;

	Vec3f Tmp = Axis1 + Axis2;
	pts[0] -= Tmp;
	pts[1] -= Tmp;
	pts[6] += Tmp;
	pts[7] += Tmp;

	Tmp = Axis1 - Axis2;
	pts[2] += Tmp;
	pts[3] += Tmp;
	pts[4] -= Tmp;
	pts[5] -= Tmp;

	addBox(pts, color, renderFlags);
}



void DebugRender::fixCapsuleVertex(Vec3f& p, float32_t radius, float32_t halfHeight)
{
	const float32_t sign = p.y > 0 ? 1.0f : -1.0f;
	p.y -= sign;
	p *= radius;
	p.y += halfHeight*sign;
}

void DebugRender::addSphere(const physx::PxVec3& sphereCenter, float sphereRadius,
	const Color8u& color, DrawFlags renderFlags)
{
	const Vec3f mySphereCenter = Vec3FromPhysx(sphereCenter);
	const size_t numVerts = NUM_CIRCLE_POINTS;
	Vec3f pts[NUM_CIRCLE_POINTS];

	if (renderFlags.IsSet(DrawFlag::WIREFRAME))
	{
		generatePolygon(pts, numVerts, Orientation::XY, sphereRadius, 0.0f);
		addCircle(pts, numVerts, color, mySphereCenter);

		generatePolygon(pts, numVerts, Orientation::XZ, sphereRadius, 0.0f);
		addCircle(pts, numVerts, color, mySphereCenter);

		generatePolygon(pts, numVerts, Orientation::YZ, sphereRadius, 0.0f);
		addCircle(pts, numVerts, color, mySphereCenter);
	}

	if (renderFlags.IsSet(DrawFlag::SOLID))
	{
		const float32_t halfHeight = 0.0f;
		for (size_t i = 0; i < gNumCapsuleIndices / 3; i++)
		{
			const uint8_t i0 = gCapsuleIndices[i * 3 + 0];
			const uint8_t i1 = gCapsuleIndices[i * 3 + 1];
			const uint8_t i2 = gCapsuleIndices[i * 3 + 2];
			Vec3f v0 = gCapsuleVertices[i0];
			Vec3f v1 = gCapsuleVertices[i1];
			Vec3f v2 = gCapsuleVertices[i2];

			fixCapsuleVertex(v0, sphereRadius, halfHeight);
			fixCapsuleVertex(v1, sphereRadius, halfHeight);
			fixCapsuleVertex(v2, sphereRadius, halfHeight);

			addTriangle(v0 + mySphereCenter, v1 + mySphereCenter, v2 + mySphereCenter, color);
		}
	}
}

void DebugRender::addBox(const physx::PxBoxGeometry& bg, const physx::PxTransform& tr,
	const Color8u& color, DrawFlags renderFlags)
{
	addOBB(tr.p, bg.halfExtents, physx::PxMat33(tr.q), color, renderFlags);
}

void DebugRender::addSphere(const physx::PxSphereGeometry& sg, const physx::PxTransform& tr, 
	const Color8u& color, DrawFlags renderFlags)
{
	addSphere(tr.p, sg.radius, color, renderFlags);
}

void DebugRender::addCone(float radius, float height, const physx::PxTransform& tr, 
	const Color8u& color, DrawFlags renderFlags)
{
	const size_t numVerts = NUM_CIRCLE_POINTS;
	Vec3f pts[NUM_CIRCLE_POINTS];

	generatePolygon(pts, numVerts, Orientation::XZ, radius, 0.0f, &tr);

	const Vec3f tip = Vec3FromPhysx(tr.transform(physx::PxVec3(0.0f, height, 0.0f)));
	const Vec3f myTrP = Vec3FromPhysx(tr.p);


	if (renderFlags.IsSet(DrawFlag::WIREFRAME))
	{
		addCircle(reinterpret_cast<Vec3f*>(pts), numVerts, color, Vec3f::zero());
		for (size_t i = 0; i<numVerts; i++)
		{
			addLine(tip, pts[i], color);	// side of the cone
			addLine(myTrP, pts[i], color);	// base disk of the cone
		}
	}

	if (renderFlags.IsSet(DrawFlag::SOLID))
	{
		for (size_t i = 0; i<numVerts; i++)
		{
			const size_t j = (i + 1) % numVerts;
			addTriangle(tip, pts[i], pts[j], color);
			addTriangle(myTrP, pts[i], pts[j], color);
		}
	}
}

void DebugRender::addSphereExt(const physx::PxVec3& sphereCenter, float sphereRadius, 
	const Color8u& color, DrawFlags renderFlags)
{
	if (renderFlags.IsSet(DrawFlag::WIREFRAME))
	{
		const size_t numVerts = NUM_CIRCLE_POINTS;
		Vec3f pts[NUM_CIRCLE_POINTS];

		generatePolygon(pts, numVerts, Orientation::XY, sphereRadius, 0.0f);
		addCircle(pts, numVerts, color, Vec3FromPhysx(sphereCenter));

		generatePolygon(pts, numVerts, Orientation::XZ, sphereRadius, 0.0f);
		addCircle(pts, numVerts, color, Vec3FromPhysx(sphereCenter));

		generatePolygon( pts, numVerts,  Orientation::YZ, sphereRadius, 0.0f);
		addCircle(pts, numVerts, color, Vec3FromPhysx(sphereCenter));
	}

	if (renderFlags.IsSet(DrawFlag::SOLID))
	{
		static bool initDone = false;
		static size_t numVerts;
		static Vec3f verts[MAX_TMP_VERT_BUF * 6];
		static Vec3f normals[MAX_TMP_VERT_BUF * 6];

		if (!initDone)
		{
			generateSphere(16, numVerts, verts, normals);
			initDone = true;
		}

		const Vec3f cen = Vec3FromPhysx(sphereCenter);

		size_t i = 0;
		while (i < numVerts)
		{
			addTriangle(
				cen + sphereRadius * verts[i],
				cen + sphereRadius * verts[i + 1],
				cen + sphereRadius * verts[i + 2],
				normals[i], normals[i + 1], normals[i + 2],
				color
			);

			i += 3;
		}
	}
}

void DebugRender::addConeExt(float radius0, float radius1, const physx::PxVec3& p0, 
	const physx::PxVec3& p1, const Color8u& color, DrawFlags renderFlags)
{
	X_ASSERT_NOT_IMPLEMENTED();
}

void DebugRender::addCylinder(float radius, float height, const physx::PxTransform& tr, 
	const Color8u& color, DrawFlags renderFlags)
{
	X_ASSERT_NOT_IMPLEMENTED();
}

void DebugRender::addStar(const physx::PxVec3& p, const float size, const Color8u& color)
{
	const Vec3f p3(p.x,p.y,p.z);
	const Vec3f up(0.f, size, 0.f);
	const Vec3f right(size, 0.f, 0.f);
	const Vec3f forwards(0.f, 0.f, size);
	addLine(p3 + up, p3 - up, color);
	addLine(p3 + right, p3 - right, color);
	addLine(p3 + forwards, p3 - forwards, color);
}

void DebugRender::addCapsule(const physx::PxVec3& p0, const physx::PxVec3& p1, 
	const float radius, const float height, const physx::PxTransform& tr,
	const Color8u& color, DrawFlags renderFlags)
{
	addSphere(p0, radius, color, renderFlags);
	addSphere(p1, radius, color, renderFlags);
	addCylinder(radius, height, tr, color, renderFlags);
}

void DebugRender::addCapsule(const physx::PxCapsuleGeometry& cg, const physx::PxTransform& tr, 
	const Color8u& color, DrawFlags renderFlags)
{
	physx::PxTransform pose = physx::PxTransform(physx::PxVec3(0.f), physx::PxQuat(physx::PxPi / 2, physx::PxVec3(0, 0, 1)));
	pose = tr * pose;

	physx::PxVec3 p0(0, -cg.halfHeight, 0);
	physx::PxVec3 p1(0, cg.halfHeight, 0);

	p0 = pose.transform(p0);
	p1 = pose.transform(p1);

	pose.p = p0;

	addCapsule(p0, p1, cg.radius, 2 * cg.halfHeight, pose, color, renderFlags);
}

void DebugRender::addGeometry(const physx::PxGeometry& geom, const physx::PxTransform& tr, 
	const Color8u& color, DrawFlags renderFlags)
{
	switch (geom.getType())
	{
	case physx::PxGeometryType::eBOX:
		addBox(static_cast<const physx::PxBoxGeometry&>(geom), tr, color, renderFlags);
		break;
	case physx::PxGeometryType::eSPHERE:
		addSphere(static_cast<const physx::PxSphereGeometry&>(geom), tr, color, renderFlags);
		break;
	case physx::PxGeometryType::eCAPSULE:
		addCapsule(static_cast<const physx::PxCapsuleGeometry&>(geom), tr, color, renderFlags);
		break;
	case physx::PxGeometryType::eCONVEXMESH:
		addConvex(static_cast<const physx::PxConvexMeshGeometry&>(geom), tr, color, renderFlags);
		break;

	case physx::PxGeometryType::ePLANE:
	case physx::PxGeometryType::eTRIANGLEMESH:
	case physx::PxGeometryType::eHEIGHTFIELD:
	default:
		X_ASSERT_NOT_IMPLEMENTED();
		break;
	}
}

void DebugRender::addRectangle(float width, float length, const physx::PxTransform& tr, 
	const Color8u& color)
{
	physx::PxMat33 m33 = physx::PxMat33(tr.q);
	Vec3f Axis1 = Vec3FromPhysx(m33.column1);
	Vec3f Axis2 = Vec3FromPhysx(m33.column2);
	const Vec3f mytrP = Vec3FromPhysx(tr.p);

	Axis1 *= length;
	Axis2 *= width;

	Vec3f pts[4];
	pts[0] = mytrP + Axis1 + Axis2;
	pts[1] = mytrP - Axis1 + Axis2;
	pts[2] = mytrP - Axis1 - Axis2;
	pts[3] = mytrP + Axis1 - Axis2;

	addTriangle(pts[0], pts[1], pts[2], color);
	addTriangle(pts[0], pts[2], pts[3], color);
}

void DebugRender::addConvex(const physx::PxConvexMeshGeometry& cg, const physx::PxTransform& tr, 
	const Color8u& color, DrawFlags renderFlags)
{
	X_ASSERT_NOT_IMPLEMENTED();
}

void DebugRender::addArrow(const physx::PxVec3& posA, const physx::PxVec3& posB, 
	const Color8u& color)
{
	const Vec3f myPosA = Vec3FromPhysx(posA);
	const Vec3f myPosB = Vec3FromPhysx(posB);

	const Vec3f t0 = Vec3FromPhysx((posB - posA).getNormalized());
	const Vec3f a = (math<float>::abs(t0.x) < 0.707f ? Vec3f(1, 0, 0) : Vec3f(0, 1, 0));
	const Vec3f t1 = t0.cross(a).normalized();
	const Vec3f t2 = t0.cross(t1).normalized();

	addLine(myPosA, myPosB, color);
	addLine(myPosB, myPosB - t0*0.15f + t1 * 0.15f, color);
	addLine(myPosB, myPosB - t0*0.15f - t1 * 0.15f, color);
	addLine(myPosB, myPosB - t0*0.15f + t2 * 0.15f, color);
	addLine(myPosB, myPosB - t0*0.15f - t2 * 0.15f, color);
}

void DebugRender::addBox(const Vec3f* pts, const Color8u& color, DrawFlags renderFlags)
{
	if (renderFlags.IsSet(DrawFlag::WIREFRAME))
	{
		const uint8_t indices[] = {
			0, 1,	1, 2,	2, 3,	3, 0,
			7, 6,	6, 5,	5, 4,	4, 7,
			1, 5,	6, 2,
			3, 7,	4, 0
		};

		for (size_t i = 0; i < 12; i++) {
			addLine(pts[indices[i * 2]], pts[indices[i * 2 + 1]], color);
		}
	}

	if (renderFlags.IsSet(DrawFlag::SOLID))
	{
		const uint8_t indices[] = {
			0,2,1,	0,3,2,
			1,6,5,	1,2,6,
			5,7,4,	5,6,7,
			4,3,0,	4,7,3,
			3,6,2,	3,7,6,
			5,0,1,	5,4,0
		};
		for (size_t i = 0; i < 12; i++) {
			addTriangle(pts[indices[i * 3 + 0]], pts[indices[i * 3 + 1]], pts[indices[i * 3 + 2]], color);
		}
	}
}

void DebugRender::addCircle(const Vec3f * pts, size_t numPts, const Color8u & color, const Vec3f & offset)
{
	for (size_t i = 0; i<numPts; i++)
	{
		const size_t j = (i + 1) % numPts;
		addLine(pts[i] + offset, pts[j] + offset, color);
	}
}


bool DebugRender::generatePolygon(physx::PxVec3* pVerts, size_t numVerts, Orientation::Enum orientation,
	float amplitude, float phase, const physx::PxTransform* pTransform)
{
	if (!pVerts || !numVerts) {
		return false;
	}

	const float step = physx::PxTwoPi / static_cast<float>(numVerts);

	for (size_t i = 0; i<numVerts; i++)
	{
		const float angle = phase + float(i) * step;
		const float y = math<float>::sin(angle) * amplitude;
		const float x = math<float>::cos(angle) * amplitude;

		if (orientation == Orientation::XY) { 
			pVerts[i] = physx::PxVec3(x, y, 0.0f);
		}
		else if (orientation == Orientation::XZ) { 
			pVerts[i] = physx::PxVec3(x, 0.0f, y);
		}
		else if (orientation == Orientation::YZ) { 
			pVerts[i] = physx::PxVec3(0.0f, x, y); 
		}
	}

	if (pTransform) {
		for (size_t i = 0; i < numVerts; i++) {
			pVerts[i] = pTransform->transform(pVerts[i]);
		}
	}

	return true;
}


bool DebugRender::generatePolygon(Vec3f* pVerts, size_t numVerts, Orientation::Enum orientation,
	float amplitude, float phase, const physx::PxTransform* pTransform)
{	
	// place your bets on this ever been a bug.
	// only way i can see it happen is if i reorder my vec or make it 16byte aligned.
	static_assert(sizeof(Vec3f) == sizeof(physx::PxVec3), "Vec sizes don't match");

	return generatePolygon(reinterpret_cast<physx::PxVec3*>(pVerts), numVerts,  orientation,
		amplitude, phase, pTransform);
}


bool DebugRender::generateSphere(size_t numSeg, size_t& numVertsOut, Vec3f* pVerts, Vec3f* pNormals)
{
	Vec3f tempVertexBuffer[MAX_TMP_VERT_BUF];
	Vec3f tempNormalBuffer[MAX_TMP_VERT_BUF];

	int32_t halfSeg = safe_static_cast<int32_t, size_t>(numSeg / 2);
	size_t numSegCheck = halfSeg * 2;

	if (((numSegCheck + 1) * (numSegCheck + 1)) > MAX_TMP_VERT_BUF) {
		return false;
	}

	const float stepTheta = physx::PxTwoPi / float(numSeg);
	const float stepPhi = physx::PxPi / static_cast<float>(numSeg);

	// compute sphere vertices on the temporary buffer
	numVertsOut = 0;
	for (size_t i = 0; i <= numSeg; i++)
	{
		const float theta = static_cast<float>(i) * stepTheta;
		const float cosi = math<float>::cos(theta);
		const float sini = math<float>::sin(theta);

		for (int32_t j = -halfSeg; j <= halfSeg; j++)
		{
			const float phi = static_cast<float>(j) * stepPhi;
			const float sinj = math<float>::sin(phi);
			const float cosj = math<float>::cos(phi);

			const float y = cosj * cosi;
			const float x = sinj;
			const float z = cosj * sini;

			tempVertexBuffer[numVertsOut] = Vec3f(x, y, z);
			tempNormalBuffer[numVertsOut] = Vec3f(x, y, z).normalized();
			numVertsOut++;
		}
	}

	numVertsOut = 0;
	// now create triangle soup data
	for (int i = 0; i < numSeg; i++)
	{
		for (int j = 0; j < numSeg; j++)
		{
			// add one triangle
			pVerts[numVertsOut] = tempVertexBuffer[(numSeg + 1) * i + j];
			pNormals[numVertsOut] = tempNormalBuffer[(numSeg + 1) * i + j];
			numVertsOut++;

			pVerts[numVertsOut] = tempVertexBuffer[(numSeg + 1) * i + j + 1];
			pNormals[numVertsOut] = tempNormalBuffer[(numSeg + 1) * i + j + 1];
			numVertsOut++;

			pVerts[numVertsOut] = tempVertexBuffer[(numSeg + 1) * (i + 1) + j + 1];
			pNormals[numVertsOut] = tempNormalBuffer[(numSeg + 1) * (i + 1) + j + 1];
			numVertsOut++;

			// add another triangle
			pVerts[numVertsOut] = tempVertexBuffer[(numSeg + 1) * i + j];
			pNormals[numVertsOut] = tempNormalBuffer[(numSeg + 1) * i + j];
			numVertsOut++;

			pVerts[numVertsOut] = tempVertexBuffer[(numSeg + 1) * (i + 1) + j + 1];
			pNormals[numVertsOut] = tempNormalBuffer[(numSeg + 1) * (i + 1) + j + 1];
			numVertsOut++;

			pVerts[numVertsOut] = tempVertexBuffer[(numSeg + 1) * (i + 1) + j];
			pNormals[numVertsOut] = tempNormalBuffer[(numSeg + 1) * (i + 1) + j];
			numVertsOut++;
		}
	}

	return true;
}

X_NAMESPACE_END
