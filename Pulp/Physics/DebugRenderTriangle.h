#pragma once


X_NAMESPACE_BEGIN(physics)

class TriangleDebugRender
{
public:
	TriangleDebugRender() = default;
	~TriangleDebugRender() = default;

public:
	void addTriangle(const Vec3f& p0, const Vec3f& p1, const Vec3f& p2, const Color8u& color);
	void addTriangle(const Vec3f& p0, const Vec3f& p1, const Vec3f& p2,
		const Vec3f& n0, const Vec3f& n1, const Vec3f& n2, const Color8u& color);

	void queueForRenderTriangle(void);

	void checkResizeTriangle(size_t maxVerts);
	void clearTriangle(void);
};


X_NAMESPACE_END
