#include "stdafx.h"
#include "DebugRenderPoints.h"


X_NAMESPACE_BEGIN(physics)

PointDebugRender::PointDebugRender() :
	numVerts_(0),
	maxVerts_(0)
{
}

void PointDebugRender::addPoint(const Vec3f& p0, const Color8u& color)
{
	checkResizePoint(numVerts_ + 1);
	addVert(p0, color);
}

void PointDebugRender::queueForRenderPoint(void)
{
}

void PointDebugRender::checkResizePoint(size_t maxVerts)
{
}

void PointDebugRender::clearPoint(void)
{
	numVerts_ = 0;
}

void PointDebugRender::addVert(const Vec3f& p, const Color8u& color)
{

}


X_NAMESPACE_END
