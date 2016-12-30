#include "stdafx.h"
#include "DebugRender.h"
#include "MathHelpers.h"

#include <IPrimativeContext.h>

X_NAMESPACE_BEGIN(physics)


DebugRender::DebugRender(engine::IPrimativeContext* pPrimCon) :
	pPrimCon_(pPrimCon)
{
	X_ASSERT_NOT_NULL(pPrimCon_);
}

DebugRender::~DebugRender()
{

}

void DebugRender::update(const physx::PxRenderBuffer& debugRenderable)
{
	// be better if we could send everything at once.
	// since each call will make auxRender resize etc.
	// might be faster to just make vectors here, populate them
	// then pass them to aux render, would use more memory but likley much faster.

	// Points
	const uint32_t numPoints = debugRenderable.getNbPoints();
	if (numPoints)
	{
		const physx::PxDebugPoint* PX_RESTRICT points = debugRenderable.getPoints();

		for (uint32_t i = 0; i<numPoints; i++)
		{
			const physx::PxDebugPoint& point = points[i];
			const Color8u col = Color8u::hexA(point.color);
			pPrimCon_->drawPoint(Vec3FromPhysx(point.pos), col);
		}
	}

	// Lines
	const uint32_t numLines = debugRenderable.getNbLines();
	if (numLines)
	{
		const physx::PxDebugLine* PX_RESTRICT lines = debugRenderable.getLines();

		for (uint32_t i = 0; i<numLines; i++)
		{
			const physx::PxDebugLine& line = lines[i];
			const Color8u col = Color8u::hexA(line.color0);
			pPrimCon_->drawLineColor(Vec3FromPhysx(line.pos0), col, Vec3FromPhysx(line.pos1), col);
		}
	}

	// Triangles
	const uint32_t numTriangles = debugRenderable.getNbTriangles();
	if (numTriangles)
	{
		const physx::PxDebugTriangle* PX_RESTRICT triangles = debugRenderable.getTriangles();

		for (uint32_t i = 0; i<numTriangles; i++)
		{
			const physx::PxDebugTriangle& triangle = triangles[i];

			pPrimCon_->drawTriangle(Vec3FromPhysx(triangle.pos0),
				Vec3FromPhysx(triangle.pos1), 
				Vec3FromPhysx(triangle.pos2), 
				Color8u::hexA(triangle.color0));
		}
	}
}

void DebugRender::queueForRender(void)
{
	// ..
}

void DebugRender::clear(void)
{
	pPrimCon_->reset();
}



X_NAMESPACE_END
