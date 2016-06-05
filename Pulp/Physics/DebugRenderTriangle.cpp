#include "stdafx.h"
#include "DebugRenderTriangle.h"


X_NAMESPACE_BEGIN(physics)


TriangleDebugRender::TriangleDebugRender() :
	numVerts_(0),
	maxVerts_(0)
{
}

void TriangleDebugRender::addTriangle(const Vec3f& p0, const Vec3f& p1,
	const Vec3f& p2, const Color8u& color)
{
	checkResizeTriangle(numVerts_ + 3);
	Vec3f normal = (p1 - p0).cross(p2 - p0);
	normal.normalize();
	addVert(p0, normal, color);
	addVert(p1, normal, color);
	addVert(p2, normal, color);
}

void TriangleDebugRender::addTriangle(const Vec3f& p0, const Vec3f& p1, const Vec3f& p2,
	const Vec3f& n0, const Vec3f& n1, const Vec3f& n2, const Color8u& color)
{
	checkResizeTriangle(numVerts_ + 3);
	addVert(p0, n0, color);
	addVert(p1, n1, color);
	addVert(p2, n2, color);
}

void TriangleDebugRender::queueForRenderTriangle(void)
{

}

void TriangleDebugRender::checkResizeTriangle(size_t maxVerts)
{

}

void TriangleDebugRender::clearTriangle(void)
{
	// don't free
	numVerts_ = 0;
}

void TriangleDebugRender::addVert(const Vec3f& p, const Vec3f& n, const Color8u& color)
{

}


X_NAMESPACE_END
