#pragma once



X_NAMESPACE_BEGIN(physics)


class LineDebugRender
{
public:
	LineDebugRender() = default;
	~LineDebugRender() = default;

public:
	void addLine(const Vec3f& p0, const Vec3f& p1, const Color8u& color);
	void checkResizeLine(size_t maxVerts);
	void queueForRenderLine(void);
	void clearLine(void);
};


X_NAMESPACE_END
