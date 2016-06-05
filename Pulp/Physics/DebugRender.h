#pragma once


#include "DebugRenderTriangle.h"
#include "DebugRenderLine.h"
#include "DebugRenderPoints.h"

X_NAMESPACE_BEGIN(physics)

class DebugRender :
	public PointDebugRender,
	public LineDebugRender,
	public TriangleDebugRender
{
	X_DECLARE_ENUM(Orientation)(
		XY,
		XZ,
		YZ
	);

	static const size_t NUM_CIRCLE_POINTS = 20;
	static const size_t NUM_CONE_POINTS = 72;
	static const size_t MAX_TMP_VERT_BUF = 400;

public:
	DebugRender();
	~DebugRender();

	X_DECLARE_FLAGS(DrawFlag) (
		WIREFRAME,
		SOLID
	);

	typedef Flags<DrawFlag> DrawFlags;

	static const DrawFlags DEFAULT_FLAGS;

public:
	void update(const physx::PxRenderBuffer& debugRenderable);
	void queueForRender();
	void clear();

	void addAABB(const physx::PxBounds3& box, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);
	void addOBB(const physx::PxVec3& boxCenter, const physx::PxVec3& boxExtents, const physx::PxMat33& boxRot, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);
	void addSphere(const physx::PxVec3& sphereCenter, float sphereRadius, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);

	void addBox(const physx::PxBoxGeometry& bg, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);

	void addSphere(const physx::PxSphereGeometry& sg, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);
	void addCone(float radius, float height, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);

	void addSphereExt(const physx::PxVec3& sphereCenter, float sphereRadius, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);
	void addConeExt(float radius0, float radius1, const physx::PxVec3& p0, const physx::PxVec3& p1, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);

	void addCylinder(float radius, float height, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);
	void addStar(const physx::PxVec3& p, const float size, const Color8u& color);

	void addCapsule(const physx::PxVec3& p0, const physx::PxVec3& p1, const float radius, const float height, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);
	void addCapsule(const physx::PxCapsuleGeometry& cg, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);

	void addGeometry(const physx::PxGeometry& geom, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);

	void addRectangle(float width, float length, const physx::PxTransform& tr, const Color8u& color);
	void addConvex(const physx::PxConvexMeshGeometry& cg, const physx::PxTransform& tr, const Color8u& color, DrawFlags renderFlags = DEFAULT_FLAGS);

	void addArrow(const physx::PxVec3& posA, const physx::PxVec3& posB, const Color8u& color);

private:
	void addBox(const Vec3f* pts, const Color8u& color, DrawFlags renderFlags);
	void addCircle(const Vec3f* pts, size_t numPts, const Color8u& color, const Vec3f& offset);

	X_INLINE static void fixCapsuleVertex(Vec3f& p, float32_t radius, float32_t halfHeight);

	static bool generatePolygon(physx::PxVec3* pVerts, size_t numVerts, Orientation::Enum orientation, 
		float amplitude, float phase, const physx::PxTransform* pTransform = nullptr);
	static bool generatePolygon(Vec3f* pVerts, size_t numVerts, Orientation::Enum orientation,
		float amplitude, float phase, const physx::PxTransform* pTransform = nullptr);

	static bool generateSphere(size_t numSeg, size_t& numVertsOut, Vec3f* pVerts, Vec3f* pNormals);
};


X_NAMESPACE_END
