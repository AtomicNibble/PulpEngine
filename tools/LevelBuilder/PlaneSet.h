#pragma once


#ifndef X_PLANE_SET_H_
#define X_PLANE_SET_H_

#include "HashIndex.h"



class XPlaneSet : public std::vector<Planef>
{
public:
	XPlaneSet() : std::vector<Planef>()
	{
		this->reserve(4096 * 8);
	}

	X_INLINE int FindPlane(const Planef& plane, const float normalEps, const float distEps);

private:
/*	struct PlaneEq
	{
		bool operator()(const Planef& s1, const Planef& s2) const
		{
			return s1.compare(s2, 0.00001f, 0.01f);
		}
	};

	struct PlaneHasher
	{
		size_t operator()(const Planef& s1) const
		{
			return (size_t)(math<float>::abs( s1.getDistance() ) * 0.125f );
		}
	};

	typedef core::HashMap<Planef, int, PlaneHasher, PlaneEq> PlaneHash;
	
	PlaneHash hash;*/

	XHashIndex hash;
};

X_INLINE int XPlaneSet::FindPlane(const Planef& plane, const float normalEps, const float distEps)
{
	int i, border, hashKey;

	hashKey = (int)(math<float>::abs(plane.getDistance()) * 0.125f);
	for (border = -1; border <= 1; border++)
	{
		for (i = hash.First(hashKey + border); i >= 0; i = hash.Next(i))
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
		hash.Add(hashKey, (int)size() - 1);
		push_back(plane);
		hash.Add(hashKey, (int)size() - 1);
		return (int)(size() - 1);;
	}
	else 
	{
		push_back(plane);
		hash.Add(hashKey, (int)size() - 1);
		push_back(-plane);
		hash.Add(hashKey, (int)size() - 1);
		return (int)(size() - 2);
	}

}


#endif // X_PLANE_SET_H_