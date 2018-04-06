#pragma once

#ifndef X_PLANE_SET_H_
#define X_PLANE_SET_H_

#include <Containers\HashIndex.h>

X_NAMESPACE_BEGIN(level)

class XPlaneSet : public core::Array<Planef>
{
public:
    XPlaneSet(core::MemoryArenaBase* arena);

    int32_t FindPlane(const Planef& plane, const float normalEps, const float distEps);

private:
    core::XHashIndex hash_;
};

X_NAMESPACE_END

#endif // X_PLANE_SET_H_