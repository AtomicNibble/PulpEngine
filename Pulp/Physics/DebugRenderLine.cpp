#include "stdafx.h"
#include "DebugRenderLine.h"


X_NAMESPACE_BEGIN(physics)


LineDebugRender::LineDebugRender() :
	numVerts_(0),
	maxVerts_(0)
{
}

void LineDebugRender::addLine(const Vec3f & p0, const Vec3f & p1, const Color8u & color)
{
	checkResizeLine(numVerts_ + 2);
	addVert(p0, color);
	addVert(p1, color);
}

void LineDebugRender::checkResizeLine(size_t maxVerts)
{
}

void LineDebugRender::queueForRenderLine(void)
{
}

void LineDebugRender::clearLine(void)
{
	numVerts_ = 0;
}

void LineDebugRender::addVert(const Vec3f& p, const Color8u& color)
{
}


X_NAMESPACE_END