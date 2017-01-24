#include "stdafx.h"
#include "Level.h"

#include "Drawing\PrimativeContext.h"

#include <Math\XWinding.h>

X_NAMESPACE_BEGIN(level)




void Level::DrawAreaBounds(void)
{
	if (s_var_drawAreaBounds_)
	{
		Color color = Col_Red;

		if (s_var_drawAreaBounds_ == 1 || s_var_drawAreaBounds_ == 3)
		{
			for (const auto& a : areas_)
			{
				if (IsAreaVisible(a))
				{
					pPrimContex_->drawAABB(a.pMesh->boundingBox, false, color);
				}
			}
		}
		else // all
		{
			for (const auto& a : areas_)
			{
				pPrimContex_->drawAABB(a.pMesh->boundingBox, false, color);
			}
		}

		if (s_var_drawAreaBounds_ > 2)
		{
			color.a = 0.2f;

			// visible only
			if (s_var_drawAreaBounds_ == 3)
			{
				for (const auto& a : areas_)
				{
					if (IsAreaVisible(a))
					{
						pPrimContex_->drawAABB(a.pMesh->boundingBox, true, color);
					}
				}
			}
			else // all
			{
				for (const auto& a : areas_)
				{
					pPrimContex_->drawAABB(a.pMesh->boundingBox, true, color);
				}
			}
		}
	}
}

void Level::DrawPortalStacks(void) const
{
	if (s_var_drawPortalStacks_)
	{
		// i wanna draw me the planes!
		// we have a clipped shape, described by a collection of planes.
		// how to i turn that into a visible shape.
		// in order to create the shape we need to clip the planes with each other.

#if 0
		for (const auto& a : areas_)
		{
			if (!IsAreaVisible(a)) {
				continue;
			}

			for (size_t x = 0; x < a.visPortalPlanes.size(); x++)
			{
				const Area::PortalPlanesArr& portalPlanes = a.visPortalPlanes[x];

				XWinding* windings[PortalStack::MAX_PORTAL_PLANES] = { nullptr };
				XWinding* w;

				size_t i, j;
				for (i = 0; i < portalPlanes.size(); i++)
				{
					const Planef& plane = portalPlanes[i];

					w = X_NEW(XWinding, g_3dEngineArena, "PortalStackDebugWinding")(plane);

					for (j = 0; j < portalPlanes.size() && w; j++)
					{
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

				for (i = 0; i < portalPlanes.size(); i++)
				{
					if (!windings[i]) {
						continue;
					}

					w = windings[i];

					size_t numPoints = w->getNumPoints();
					for (j = 0; j < numPoints; j++)
					{
						Vec3f start = (*w)[j].asVec3();
						Vec3f end = (*w)[(j + 1) % numPoints].asVec3();
						pPrimContex_->drawLine(start, end, col);
					}
				}

				for (i = 0; i < portalPlanes.size(); i++)
				{
					if (!windings[i]) {
						continue;
					}
					X_DELETE(windings[i], g_3dEngineArena);
				}
			}
		}

#endif
	}
}

void Level::DrawStatsBlock(void) const
{
	if (!s_var_drawStats_) {
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