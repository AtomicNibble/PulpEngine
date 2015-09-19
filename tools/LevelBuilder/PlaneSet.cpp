#include "stdafx.h"
#include "PlaneSet.h"


XPlaneSet::XPlaneSet() : core::Array<Planef>(g_arena), hash(g_arena)
{
	this->reserve(4096 * 8);
}


int XPlaneSet::FindPlane(const Planef& plane, const float normalEps, const float distEps)
{
	int i, border, hashKey;

	hashKey = (int)(math<float>::abs(plane.getDistance()) * 0.125f);
	for (border = -1; border <= 1; border++)
	{
		for (i = hash.first(hashKey + border); i >= 0; i = hash.next(i))
		{
			if ((*this)[i].compare(plane, normalEps, distEps)) {
				return i;
			}
		}
	}

	PlaneType::Enum type = plane.getType();

	if (type >= PlaneType::NEGX && PlaneType::isTrueAxial(type))
	{
		push_back(-plane);
		hash.add(hashKey, static_cast<int>(size()) - 1);
		push_back(plane);
		hash.add(hashKey, static_cast<int>(size()) - 1);
		return static_cast<int>(size() - 1);
	}
	else
	{
		push_back(plane);
		hash.add(hashKey, static_cast<int>(size()) - 1);
		push_back(-plane);
		hash.add(hashKey, static_cast<int>(size()) - 1);
		return static_cast<int>(size() - 2);
	}
}
