#pragma once



X_NAMESPACE_BEGIN(physics)


class LineDebugRender
{
	X_NO_COPY(LineDebugRender);

public:
	LineDebugRender();
	~LineDebugRender() = default;

public:
	void addLine(const Vec3f& p0, const Vec3f& p1, const Color8u& color);
	void checkResizeLine(size_t maxVerts);
	void queueForRenderLine(void);
	void clearLine(void);

private:
	void addVert(const Vec3f& p, const Color8u& color);


private:
	size_t numVerts_;
	size_t maxVerts_;
};


X_NAMESPACE_END
