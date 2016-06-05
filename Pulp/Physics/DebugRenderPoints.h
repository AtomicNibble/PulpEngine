#pragma once



X_NAMESPACE_BEGIN(physics)

class PointDebugRender
{
	X_NO_COPY(PointDebugRender);

public:
	PointDebugRender();
	~PointDebugRender() = default;

public:
	void addPoint(const Vec3f& p0, const Color8u& color);

	void queueForRenderPoint(void);
	void checkResizePoint(size_t maxVerts);
	void clearPoint(void);

private:
	void addVert(const Vec3f& p, const Color8u& color);

private:
	size_t numVerts_;
	size_t maxVerts_;
};


X_NAMESPACE_END
