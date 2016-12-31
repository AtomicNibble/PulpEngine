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
		const physx::PxDebugPoint* X_RESTRICT points = debugRenderable.getPoints();

		engine::IPrimativeContext::PrimVertex* X_RESTRICT pPoints = pPrimCon_->addPrimative(numPoints,
			engine::IPrimativeContext::PrimitiveType::POINTLIST);

		for (uint32_t i = 0; i<numPoints; i++)
		{
			const physx::PxDebugPoint& point = points[i];

			pPoints[i].pos = Vec3FromPhysx(point.pos);
			pPoints[i].color = Color8u::hexA(point.color);
		}
	}

	// Lines
	const uint32_t numLines = debugRenderable.getNbLines();
	if (numLines)
	{
		const physx::PxDebugLine* X_RESTRICT lines = debugRenderable.getLines();

		// lets do the batching ourself, since we need to cast so we can't use the range submit.
		engine::IPrimativeContext::PrimVertex* X_RESTRICT pLines = pPrimCon_->addPrimative(2 * numLines,
			engine::IPrimativeContext::PrimitiveType::LINELIST);

		for (uint32_t i = 0; i<numLines; i++)
		{
			const physx::PxDebugLine& line = lines[i];

			pLines[0].pos = Vec3FromPhysx(line.pos0);
			pLines[0].color = Color8u::hexA(line.color0);
			pLines[1].pos = Vec3FromPhysx(line.pos1);
			pLines[1].color = Color8u::hexA(line.color1);
			pLines += 2;
		}
	}

	// Triangles
	const uint32_t numTriangles = debugRenderable.getNbTriangles();
	if (numTriangles)
	{
		const physx::PxDebugTriangle* X_RESTRICT triangles = debugRenderable.getTriangles();

		engine::IPrimativeContext::PrimVertex* X_RESTRICT pVerts = pPrimCon_->addPrimative(3 * numTriangles,
			engine::IPrimativeContext::PrimitiveType::TRIANGLELIST);

		for (uint32_t i = 0; i<numTriangles; i++)
		{
			const physx::PxDebugTriangle& triangle = triangles[i];
			auto* pTri = &pVerts[i * 3];

			pTri[0].pos = Vec3FromPhysx(triangle.pos0);
			pTri[0].color = Color8u::hexA(triangle.color0);
			pTri[1].pos = Vec3FromPhysx(triangle.pos1);
			pTri[1].color = Color8u::hexA(triangle.color1);
			pTri[2].pos = Vec3FromPhysx(triangle.pos2);
			pTri[2].color = Color8u::hexA(triangle.color2);
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
