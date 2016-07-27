#include "stdafx.h"
#include "DebugRender.h"
#include "MathHelpers.h"

#include <IRenderAux.h>

X_NAMESPACE_BEGIN(physics)


DebugRender::DebugRender(render::IRenderAux* pAuxRender) :
	pAuxRender_(pAuxRender)
{
	X_ASSERT_NOT_NULL(pAuxRender_);
}

DebugRender::~DebugRender()
{

}

void DebugRender::update(const physx::PxRenderBuffer& debugRenderable)
{
	render::IRenderAux& aux = *pAuxRender_;

	// Points
	const uint32_t numPoints = debugRenderable.getNbPoints();
	if (numPoints)
	{
		X_ASSERT_NOT_IMPLEMENTED();
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
			aux.drawLine(Vec3FromPhysx(line.pos0), col, Vec3FromPhysx(line.pos1), col);
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

			aux.drawTriangle(Vec3FromPhysx(triangle.pos0),
				Vec3FromPhysx(triangle.pos1), 
				Vec3FromPhysx(triangle.pos2), 
				Color8u::hexA(triangle.color0));
		}
	}
}

void DebugRender::queueForRender(void)
{
	pAuxRender_->flush();
}

void DebugRender::clear(void)
{
	pAuxRender_->clear();
}



X_NAMESPACE_END
