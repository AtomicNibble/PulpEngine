#include "stdafx.h"
#include "DebugRender.h"
#include "Util/MathHelpers.h"

#include <IPrimitiveContext.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(physics)

DebugRender::DebugRender(engine::IPrimitiveContext* pPrimCon) :
    pPrimCon_(pPrimCon)
{
    X_ASSERT_NOT_NULL(pPrimCon_);
}

DebugRender::~DebugRender()
{
}

void DebugRender::update(const physx::PxRenderBuffer& debugRenderable)
{
    // We need to handle the case where any of the primitives take more verts than a given page.
    // we can handle the case where points lines and triangles use almost a page each just fine
    // it's just if one of them is bigger than a single page we will crash due to access violation.
    // the ideal way to handle this is for the primContex to tell us how many verts we got back that way we can spread
    // the prims across multiple pages if required.
    // or what if we just know the answer and break it up ourselves, since this is the only place that requires this logic.
    // so might as well just write it here.
    // Note: done.

    const uint32_t maxVertsPerBatch = safe_static_cast<uint32_t>(pPrimCon_->maxVertsPerPrim());

    // Points
    uint32_t numPoints = debugRenderable.getNbPoints();
    if (numPoints) {
        const physx::PxDebugPoint* X_RESTRICT points = debugRenderable.getPoints();

        while (numPoints) {
            const uint32_t pointBatchSize = core::Min(maxVertsPerBatch, numPoints);

            engine::IPrimitiveContext::PrimVertex* X_RESTRICT pPoints = pPrimCon_->addPrimitive(pointBatchSize,
                engine::IPrimitiveContext::PrimitiveType::POINTLIST);

            for (uint32_t i = 0; i < pointBatchSize; i++) {
                const physx::PxDebugPoint& point = points[i];

                pPoints[i].pos = Vec3FromPx3(point.pos);
                pPoints[i].color = Color8u::hex(point.color);
            }

            points += pointBatchSize;
            numPoints -= pointBatchSize;
        }
    }

    // Lines
    uint32_t numLines = debugRenderable.getNbLines();
    if (numLines) {
        const physx::PxDebugLine* X_RESTRICT lines = debugRenderable.getLines();

        // lets do the batching ourselves, since we need to cast so we can't use the range submit.
        while (numLines) {
            const uint32_t lineBatchVertSize = core::Min(maxVertsPerBatch, numLines * 2);
            const uint32_t lineBatchSize = lineBatchVertSize >> 1;

            X_ASSERT(lineBatchSize * 2 == lineBatchVertSize, "")();

            engine::IPrimitiveContext::PrimVertex* X_RESTRICT pLines = pPrimCon_->addPrimitive(lineBatchVertSize,
                engine::IPrimitiveContext::PrimitiveType::LINELIST);

            for (uint32_t i = 0; i < lineBatchSize; i++) {
                const physx::PxDebugLine& line = lines[i];

                pLines[0].pos = Vec3FromPx3(line.pos0);
                pLines[0].color = Color8u::hex(line.color0);
                pLines[1].pos = Vec3FromPx3(line.pos1);
                pLines[1].color = Color8u::hex(line.color1);
                pLines += 2;
            }

            lines += lineBatchSize;
            numLines -= lineBatchSize;
        }
    }

    // Triangles
    uint32_t numTriangles = debugRenderable.getNbTriangles();
    if (numTriangles) {
        const physx::PxDebugTriangle* X_RESTRICT triangles = debugRenderable.getTriangles();

        while (numTriangles) {
            const uint32_t triBatchVertSize = core::Min(maxVertsPerBatch, numTriangles * 3);
            const uint32_t triBatchSize = triBatchVertSize / 3;

            X_ASSERT(triBatchSize * 3 == triBatchVertSize, "")();

            engine::IPrimitiveContext::PrimVertex* X_RESTRICT pVerts = pPrimCon_->addPrimitive(triBatchVertSize,
                engine::IPrimitiveContext::PrimitiveType::TRIANGLELIST);

            for (uint32_t i = 0; i < triBatchSize; i++) {
                const physx::PxDebugTriangle& triangle = triangles[i];
                auto* pTri = pVerts;

                pTri[0].pos = Vec3FromPx3(triangle.pos0);
                pTri[0].color = Color8u::hex(triangle.color0);

                pTri[1].pos = Vec3FromPx3(triangle.pos1);
                pTri[1].color = Color8u::hex(triangle.color1);

                pTri[2].pos = Vec3FromPx3(triangle.pos2);
                pTri[2].color = Color8u::hex(triangle.color2);

                pVerts += 3;
            }

            triangles += triBatchSize;
            numTriangles -= triBatchSize;
        }
    }

    // Text
    uint32_t numText = debugRenderable.getNbTexts();
    if (numText) {
        X_ASSERT_NOT_IMPLEMENTED(); // check this logic below is correct.

        const physx::PxDebugText* X_RESTRICT pTexts = debugRenderable.getTexts();

        font::TextDrawContext con;

        for (uint32_t i = 0; i < numText; i++) {
            const auto& text = pTexts[i];

            con.size.x = text.size;
            con.size.y = text.size;
            con.col = Color8u::hex(text.color);

            pPrimCon_->drawText(Vec3FromPx3(text.position), con, text.string);
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
