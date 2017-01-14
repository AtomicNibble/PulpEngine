#include "stdafx.h"
#include "Level.h"

#include <IFileSys.h>
#include <IRender.h>
#include <IRenderMesh.h>
#include <ITimer.h>

#include <Memory\MemCursor.h>
#include <Math\XWinding.h>
#include "Drawing\PrimativeContext.h"

X_NAMESPACE_BEGIN(level)

void Level::clearVisPortals(void)
{
	for (auto& a : areas_)
	{
		a.visPortalPlanes.clear();
	}
}


void Level::FloodVisibleAreas(void)
{
	if (s_var_drawArea_ == -1)
	{
		const XCamera& cam = cam_;
		const Vec3f camPos = cam.getPosition();

		camArea_ = -1;
		outsideWorld_ = !IsPointInAnyArea(camPos, camArea_);


		// if we are outside world draw all.
		// or usePortals is off.
		if (camArea_ < 0 || s_var_usePortals_ == 0)
		{
			// add all areas
			AreaArr::ConstIterator it = areas_.begin();
			for (; it != areas_.end(); ++it)
			{
				SetAreaVisible(it->areaNum);
			}
		}
		else
		{
			SetAreaVisible(camArea_);

			if (s_var_drawCurrentAreaOnly_ != 1)
			{
				if (!AreaHasPortals(camArea_)) {
					return;
				}

				XWinding w;

				// build up the cams planes.
				const Planef camPlanes[FrustumPlane::ENUM_COUNT] = {
					cam.getFrustumPlane(FrustumPlane::LEFT),
					cam.getFrustumPlane(FrustumPlane::RIGHT),
					cam.getFrustumPlane(FrustumPlane::TOP),
					cam.getFrustumPlane(FrustumPlane::BOTTOM),
					cam.getFrustumPlane(FrustumPlane::FAR),
					cam.getFrustumPlane(FrustumPlane::NEAR)
				};

				const Planef farPlane = cam.getFrustumPlane(FrustumPlane::FAR);

				PortalStack	ps;
				size_t i, j;

				for (i = 0; i < FrustumPlane::ENUM_COUNT; i++) {
					ps.portalPlanes.append(camPlanes[i]);
				}


				// work out what other areas we can see from this one.
				const Area& area = areas_[camArea_];

				Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
				for (; apIt != area.portals.end(); ++apIt)
				{
					const AreaPortal& portal = *apIt;

					// make sure this portal is facing away from the view
					float dis = portal.plane.distance(camPos);
					if (dis < -0.1f) {
						continue;
					}


					if (dis < 1.0f) {
						// go through this portal
						PortalStack	newStack;
						newStack = ps;
						newStack.pPortal = &portal;
						newStack.pNext = &ps;
						FloodViewThroughArea_r(camPos, portal.areaTo, farPlane, &newStack);
						continue;
					}

					// work out if the portal is visible, by clipping the portals winding with that of the cam planes.
					w = (*portal.pWinding);

					for (j = 0; j < FrustumPlane::ENUM_COUNT; j++) {
						if (!w.clipInPlace(camPlanes[j], 0)) {
							break;
						}
					}
					if (!w.getNumPoints()) {
						continue;	// portal not visible
					}

					// go through this portal
					PortalStack	newStack;
					newStack.pPortal = &portal;
					newStack.pNext = &ps;

					Vec3f v1, v2;

					size_t addPlanes = w.getNumPoints();
					if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
						addPlanes = PortalStack::MAX_PORTAL_PLANES;
					}

					for (i = 0; i < addPlanes; i++)
					{
						j = i + 1;
						if (j == w.getNumPoints()) {
							j = 0;
						}

						v1 = camPos - w[i].asVec3();
						v2 = camPos - w[j].asVec3();

						Vec3f normal = v2.cross(v1);
						normal.normalize();

						// if it is degenerate, skip the plane
						if (normal.length() < 0.01f) {
							continue;
						}

						Planef& plane = newStack.portalPlanes.AddOne();
						plane.setNormal(normal);
						plane.setDistance(normal * camPos);
						plane = -plane;
					}

					// far plane
					newStack.portalPlanes.append(farPlane); 

					// the last stack plane is the portal plane
					newStack.portalPlanes.append(-portal.plane);

					FloodViewThroughArea_r(camPos, portal.areaTo, farPlane, &newStack);

				}
			}
		}
	}
	else if (s_var_drawArea_ < safe_static_cast<int, size_t>(areas_.size()))
	{
		// force draw just this area even if outside world.
		SetAreaVisible(s_var_drawArea_);
	}
}


void Level::DrawVisibleAreas(void)
{
	AreaArr::ConstIterator it = areas_.begin();

	for (; it != areas_.end(); ++it)
	{
		const Area& area = *it;

		if (IsAreaVisible(area)) {
			DrawArea(area);
		}
	}
}


// ==================================================================

size_t Level::NumPortalsInArea(int32_t areaNum) const
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	return areas_[areaNum].portals.size();
}

bool Level::AreaHasPortals(int32_t areaNum) const
{
	return NumPortalsInArea(areaNum) > 0;
}


void Level::FlowViewThroughPortals(const int32_t areaNum, const Vec3f origin,
	size_t numPlanes, const Planef* pPlanes)
{
	PortalStack	ps;
	size_t	i;

	// copy the planes.
	for (i = 0; i < numPlanes; i++) {
		ps.portalPlanes.append(pPlanes[i]);
	}

	if (areaNum < 0)
	{
		// outside draw everything.
		for (i = 0; i < areas_.size(); i++) {
			SetAreaVisible(safe_static_cast<int32_t>(i), &ps);
		}
	}
	else
	{
		const XCamera& cam = cam_;
		const Planef farPlane = cam.getFrustumPlane(FrustumPlane::FAR);

		FloodViewThroughArea_r(origin, areaNum, farPlane, &ps);
	}
}


void Level::FloodViewThroughArea_r(const Vec3f origin, int32_t areaNum, const Planef& farPlane,
	const PortalStack* ps)
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	PortalStack	newStack;
	const PortalStack* check;
	XWinding w;
	float dis;
	size_t j;

	const Area& area = areas_[areaNum];

	// add this area.
	SetAreaVisible(areaNum, ps);

	// see what portals we can see.

	Area::AreaPortalArr::ConstIterator apIt = area.portals.begin();
	for (; apIt != area.portals.end(); ++apIt)
	{
		const AreaPortal& portal = *apIt;

		// make sure this portal is facing away from the view
		dis = portal.plane.distance(origin);
		if (dis < -0.1f) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for (check = ps; check; check = check->pNext) {
			if (check->pPortal == &portal) {
				break;	// don't recursively enter a stack
			}
		}
		if (check) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if (dis < 1.0f)
		{
			// go through this portal
			newStack = *ps;
			newStack.pPortal = &portal;
			newStack.pNext = ps;
			FloodViewThroughArea_r(origin, portal.areaTo, farPlane, &newStack);
			continue;
		}

		// clip the portal winding to all of the planes
		w = *portal.pWinding;

		for (j = 0; j < ps->portalPlanes.size(); j++)
		{
			if (!w.clipInPlace(ps->portalPlanes[j], 0))
			{
				break;
			}
		}
		if (!w.getNumPoints()) {
			continue;	// portal not visible
		}

		// go through this portal
		newStack.pPortal = &portal;
		newStack.pNext = ps;
		newStack.portalPlanes.clear();

		size_t i, addPlanes;
		Vec3f	v1, v2;

		addPlanes = w.getNumPoints();
		if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
			addPlanes = PortalStack::MAX_PORTAL_PLANES;
		}

		for (i = 0; i < addPlanes; i++)
		{
			j = i + 1;
			if (j == w.getNumPoints()) {
				j = 0;
			}

			v1 = origin - w[i].asVec3();
			v2 = origin - w[j].asVec3();

			Vec3f normal = v2.cross(v1);
			normal.normalize();

			// if it is degenerate, skip the plane
			if (normal.length() < 0.01f) {
				continue;
			}

			Planef& plane = newStack.portalPlanes.AddOne();
			plane.setNormal(normal);
			plane.setDistance(normal * origin);
			plane = -plane;
		}

		newStack.portalPlanes.append(farPlane);

		// the last stack plane is the portal plane
		newStack.portalPlanes.append(-portal.plane);

		FloodViewThroughArea_r(origin, portal.areaTo, farPlane, &newStack);
	}
}


void Level::SetAreaVisible(int32_t areaNum, const PortalStack* ps)
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	// make a copy of the portal stack.
	// it's used for ent culling later.
	Area& area = areas_[areaNum];

	area.visPortalPlanes.append(ps->portalPlanes);

	SetAreaVisible(areaNum);
}

void Level::DrawPortalDebug(void) const
{
	using namespace render;

	if (s_var_drawPortals_ > 0 && !outsideWorld_)
	{
#if 0
		XAuxGeomRenderFlags flags = AuxGeom_Defaults::Def3DRenderflags;

		// 0=off 1=solid 2=wire 3=solid_dt 4=wire_dt
		flags.SetAlphaBlendMode(AuxGeom_AlphaBlendMode::AlphaBlended);

		if (s_var_drawPortals_ == 2 || s_var_drawPortals_ == 4)
		{
			flags.SetFillMode(AuxGeom_FillMode::FillModeWireframe);
		}

		if (s_var_drawPortals_ == 3 || s_var_drawPortals_ == 4)
		{
			flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOn);
			flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOn);
		}
		else
		{
			flags.SetDepthWriteFlag(AuxGeom_DepthWrite::DepthWriteOff);
			flags.SetDepthTestFlag(AuxGeom_DepthTest::DepthTestOff);
		}

		flags.SetCullMode(AuxGeom_CullMode::CullModeNone);
#endif

	//	pAux_->setRenderFlags(flags);


		// draw the portals.
		AreaArr::ConstIterator areaIt = areas_.begin();
		for (; areaIt != areas_.end(); ++areaIt)
		{
			if (!IsAreaVisible(*areaIt)) {
				continue;
			}

			Area::AreaPortalArr::ConstIterator apIt = areaIt->portals.begin();
			for (; apIt != areaIt->portals.end(); ++apIt)
			{
				const AreaPortal& portal = *apIt;

#if 1
				if (IsAreaVisible(areas_[portal.areaTo]))
				{
					pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
						portal.debugVerts.size(), Colorf(0.f, 1.f, 0.f, 0.35f));
				}
				else
				{
					pPrimContex_->drawTriangle(portal.debugVerts.ptr(),
						portal.debugVerts.size(), Colorf(1.f, 0.f, 0.f, 0.3f));
				}
#else
		

				AABB box;
				portal.pWinding->GetAABB(box);

				if (areas_[portal.areaTo].frameID == frameID_)
				{
					pAux_->drawAABB(
						box, Vec3f::zero(), true, Colorf(0.f, 1.f, 0.f, 0.45f)
						);
				}
				else
				{
					pAux_->drawAABB(
						box, Vec3f::zero(), true, Colorf(1.f, 0.f, 0.f, 1.f)
						);
				}
#endif
			}
		}
	}
}

X_NAMESPACE_END