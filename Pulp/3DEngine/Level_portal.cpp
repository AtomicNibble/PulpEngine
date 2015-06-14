#include "stdafx.h"
#include "Level.h"

#include <IFileSys.h>
#include <IRenderMesh.h>
#include <ITimer.h>

#include <Memory\MemCursor.h>
#include <Math\XWinding.h>


X_NAMESPACE_BEGIN(level)

size_t Level::NumPortalsInArea(int32_t areaNum) const
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	return areas_[areaNum].portals.size();
}


void Level::FlowViewThroughPortals(const int32_t areaNum, const Vec3f origin,
	size_t numPlanes, const Planef* pPlanes)
{
	PortalStack	ps;
	size_t	i;

	// copy the planes.
	for (i = 0; i < numPlanes; i++) {
		ps.portalPlanes[i] = pPlanes[i];
	}
	ps.numPortalPlanes = numPlanes;

	if (areaNum < 0)
	{
		// outside draw everything.
		for (i = 0; i < areas_.size(); i++) {
			AddAreaRefs(i, &ps);
		}
	}
	else
	{
		FloodViewThroughArea_r(origin, areaNum, &ps);
	}
}


void Level::FloodViewThroughArea_r(const Vec3f origin, int32_t areaNum,
	const PortalStack* ps)
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	PortalStack	newStack;
	const PortalStack* check;
	XWinding w;
	float dis;
	size_t j;

	Area& area = areas_[areaNum];

	// add this area.
	AddAreaRefs(areaNum, ps);

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

		X_LOG0_EVERY_N(24, "Level", "distance from area %i plane: %f",
			portal.areaTo, dis);

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if (dis < 1.0f)
		{
			// go through this portal
			newStack = *ps;
			newStack.pPortal = &portal;
			newStack.pNext = ps;
			FloodViewThroughArea_r(origin, portal.areaTo, &newStack);
			continue;
		}

		// clip the portal winding to all of the planes
		w = *portal.pWinding;

		Vec3f points[8];

		for (j = 0; j < ps->numPortalPlanes; j++)
		{
			for (int32_t x = 0; x < w.getNumPoints(); x++) {
				points[x] = w[x].asVec3();
			}

			if (!w.clipInPlace(-ps->portalPlanes[j], 0))
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

		int i, addPlanes;
		Vec3f	v1, v2;

		addPlanes = w.getNumPoints();
		if (addPlanes > PortalStack::MAX_PORTAL_PLANES) {
			addPlanes = PortalStack::MAX_PORTAL_PLANES;
		}

		newStack.numPortalPlanes = 0;
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

			newStack.portalPlanes[newStack.numPortalPlanes].setNormal(normal);
			newStack.portalPlanes[newStack.numPortalPlanes].setDistance((normal * origin));

			newStack.numPortalPlanes++;
		}

		// the last stack plane is the portal plane
		newStack.portalPlanes[newStack.numPortalPlanes] = portal.plane;
		newStack.numPortalPlanes++;

		FloodViewThroughArea_r(origin, portal.areaTo, &newStack);
	}
}


void Level::AddAreaRefs(int32_t areaNum, const PortalStack* ps)
{
	X_ASSERT((areaNum >= 0 && areaNum < safe_static_cast<int32_t, size_t>(NumAreas())),
		"areaNum out of range")(areaNum, NumAreas());

	areas_[areaNum].frameID = frameID_;
}



X_NAMESPACE_END