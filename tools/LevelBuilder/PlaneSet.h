#pragma once


#ifndef X_PLANE_SET_H_
#define X_PLANE_SET_H_

#include <Containers\HashIndex.h>

class XPlaneSet : public std::vector<Planef>
{
public:
	XPlaneSet();

	int FindPlane(const Planef& plane, const float normalEps, const float distEps);

private:

	core::XHashIndex hash;
};


#endif // X_PLANE_SET_H_