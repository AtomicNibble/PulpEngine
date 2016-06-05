#pragma once



X_NAMESPACE_BEGIN(physics)

class PointDebugRender
{
public:
	PointDebugRender() = default;
	~PointDebugRender() = default;

public:
	void addPoint(const Vec3f& p0, const Color8u& color);

	void queueForRenderPoint(void);
	void checkResizePoint(size_t maxVerts);
	void clearPoint(void);
};


X_NAMESPACE_END
