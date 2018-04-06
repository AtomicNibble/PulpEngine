#include "stdafx.h"
#include "Level.h"

#include "Drawing\PrimativeContext.h"

#include <Math\XWinding.h>

X_NAMESPACE_BEGIN(level)

void Level::DebugDraw_AreaBounds(void) const
{
    if (vars_.drawAreaBounds()) {
        Color color = Col_Red;

        if (vars_.drawAreaBounds() == 1 || vars_.drawAreaBounds() == 3) {
            for (const auto& a : areas_) {
                if (IsAreaVisible(a)) {
                    pPrimContex_->drawAABB(a.pMesh->boundingBox, false, color);
                }
            }
        }
        else // all
        {
            for (const auto& a : areas_) {
                pPrimContex_->drawAABB(a.pMesh->boundingBox, false, color);
            }
        }

        if (vars_.drawAreaBounds() > 2) {
            color.a = 0.2f;

            // visible only
            if (vars_.drawAreaBounds() == 3) {
                for (const auto& a : areas_) {
                    if (IsAreaVisible(a)) {
                        pPrimContex_->drawAABB(a.pMesh->boundingBox, true, color);
                    }
                }
            }
            else // all
            {
                for (const auto& a : areas_) {
                    pPrimContex_->drawAABB(a.pMesh->boundingBox, true, color);
                }
            }
        }
    }
}

void Level::DebugDraw_Portals(void) const
{
    if (vars_.drawPortals() > 0 /* && !outsideWorld_ */) {
        // draw the portals.
        AreaArr::ConstIterator areaIt = areas_.begin();
        for (; areaIt != areas_.end(); ++areaIt) {
            if (!IsAreaVisible(*areaIt)) {
                continue;
            }

            if (vars_.drawPortals() > 1) {
                pPrimContex_->setDepthTest(1);
            }

            Area::AreaPortalArr::ConstIterator apIt = areaIt->portals.begin();
            for (; apIt != areaIt->portals.end(); ++apIt) {
                const AreaPortal& portal = *apIt;

                if (IsAreaVisible(areas_[portal.areaTo])) {
                    pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
                        portal.debugVerts.size(), Colorf(0.f, 1.f, 0.f, 0.35f));
                }
                else {
                    pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
                        portal.debugVerts.size(), Colorf(1.f, 0.f, 0.f, 0.3f));
                }
            }

            if (vars_.drawPortals() > 1) {
                pPrimContex_->setDepthTest(0);
            }
        }
    }
}

void Level::DebugDraw_PortalStacks(void) const
{
    if (vars_.drawPortalStacks()) {
        // i wanna draw me the planes!
        // we have a clipped shape, described by a collection of planes.
        // how to i turn that into a visible shape.
        // in order to create the shape we need to clip the planes with each other.

        for (const auto& a : areas_) {
            if (!IsAreaVisible(a)) {
                continue;
            }

            for (const auto& vp : a.visPortals) {
                const auto& portalPlanes = vp.planes;

                XWinding* windings[PortalStack::MAX_PORTAL_PLANES] = {nullptr};
                XWinding* w;

                size_t i, j;
                for (i = 0; i < portalPlanes.size(); i++) {
                    const Planef& plane = portalPlanes[i];

                    w = X_NEW(XWinding, g_3dEngineArena, "PortalStackDebugWinding")(plane);

                    for (j = 0; j < portalPlanes.size() && w; j++) {
                        if (i == j) {
                            continue;
                        }

                        if (!w->clip(portalPlanes[j], 0.01f)) {
                            X_DELETE_AND_NULL(w, g_3dEngineArena);
                        }
                    }

                    windings[i] = w;
                }

                Color8u col = Col_Limegreen;

                for (i = 0; i < portalPlanes.size(); i++) {
                    if (!windings[i]) {
                        continue;
                    }

                    w = windings[i];

                    size_t numPoints = w->getNumPoints();
                    for (j = 0; j < numPoints; j++) {
                        Vec3f start = (*w)[j].asVec3();
                        Vec3f end = (*w)[(j + 1) % numPoints].asVec3();
                        pPrimContex_->drawLine(start, end, col);
                    }
                }

                for (i = 0; i < portalPlanes.size(); i++) {
                    if (!windings[i]) {
                        continue;
                    }
                    X_DELETE(windings[i], g_3dEngineArena);
                }
            }
        }
    }
}

void Level::DebugDraw_StaticModelCullVis(void) const
{
    if (vars_.drawModelBounds()) {
        const Color8u cullColor(128, 0, 0, 128);
        const Color8u visColor(255, 255, 64, 128);

        for (const auto& a : areas_) {
            if (!IsAreaVisible(a)) {
                continue;
            }

            if (vars_.drawModelBounds() > 2) {
                // draw what was culled.
                const FileAreaRefHdr& areaModelsHdr = modelRefs_.areaRefHdrs[a.areaNum];
                size_t i = areaModelsHdr.startIndex;
                size_t end = i + areaModelsHdr.num;

                for (; i < end; i++) {
                    uint32_t entId = modelRefs_.areaRefs[i].modelId;

                    if (!std::binary_search(a.areaVisibleEnts.begin(), a.areaVisibleEnts.end(), entId)) {
                        const level::StaticModel& sm = staticModels_[entId - 1];

                        pPrimContex_->drawAABB(sm.boundingBox, false, cullColor);
                    }
                }
            }

            {
                // draw whats visible.

                for (const auto id : a.areaVisibleEnts) {
                    const level::StaticModel& sm = staticModels_[id - 1];

                    pPrimContex_->drawAABB(sm.boundingBox, false, visColor);
                }
            }
        }
    }
}

void Level::DebugDraw_ModelBones(void) const
{
    if (vars_.drawModelBones()) {
#if 0
		const Color8u visColor(255, 255, 64, 128);

		for (const auto& a : areas_)
		{
			if (!IsAreaVisible(a)) {
				continue;
			}

			for (const auto id : a.areaVisibleEnts)
			{
				const level::StaticModel& sm = staticModels_[id - 1];

				Matrix44f posMat = Matrix44f::createTranslation(sm.pos);
				posMat.rotate(sm.angle.getAxis(), sm.angle.getAngle());

				// save a call?
				if (sm.pModel->numBones() > 0)
				{
					sm.pModel->RenderBones(pPrimContex_, posMat, Col_Royalblue);
				}
			}
		}
#endif
    }
}

void Level::DebugDraw_DrawDetachedCam(void) const
{
    if (vars_.detachCam() > 0) {
        if (vars_.detachCam() == 1) {
            pPrimContex_->drawFrustum(cam_, Color8u(255, 255, 255, 128), Color8u(200, 0, 0, 100), true);
        }
        else {
            pPrimContex_->drawFrustum(cam_, Color8u(255, 255, 255, 255), Color8u(200, 0, 0, 255), false);
        }
    }
}

void Level::DrawStatsBlock(void) const
{
    if (!vars_.drawStats()) {
        return;
    }

    // needs re doing with a prim context.
#if 0
	pRender_->Set2D(true);
	{
		core::StackString512 str;

		str.appendFmt("NumAreas:%" PRIuS "\n", areas_.size());
		str.appendFmt("VisibleAreas:%" PRIuS "\n", frameStats_.visibleAreas);
		str.appendFmt("VisibleModels:%" PRIuS "\n", frameStats_.visibleModels);
		str.appendFmt("CulledModel:%" PRIuS "\n", frameStats_.culledModels);
		str.appendFmt("VisibleVerts:%" PRIuS "\n", frameStats_.visibleVerts);
		str.appendFmt("VisibleEnts:%" PRIuS "\n", frameStats_.visibleEnts);

		Color txt_col(0.7f, 0.7f, 0.7f, 1.f);
		const float height = 120.f;
		const float width = 200.f;

		float screenWidth = pRender_->getWidthf();

		Vec2f pos(screenWidth - (width + 5.f), 35.f);

		pRender_->DrawQuad(pos.x, pos.y, width, height, Color(0.1f, 0.1f, 0.1f, 0.8f),
			Color(0.01f, 0.01f, 0.01f, 0.95f));

		{
			render::DrawTextInfo ti;
			ti.col = txt_col;
			ti.flags = render::DrawTextFlags::POS_2D | render::DrawTextFlags::MONOSPACE;

			{
				Vec3f txtPos(pos.x + 5, pos.y + 20, 1);
				pRender_->DrawText(txtPos, ti, str.c_str());
			}

			{
				Vec3f txtPos(pos.x + (width / 2), pos.y, 1);
				ti.flags |= render::DrawTextFlags::CENTER;
				pRender_->DrawText(txtPos, ti, "Level Draw Stats");
			}
		}
	}
	pRender_->Set2D(false);
#endif
}

X_NAMESPACE_END