#include "stdafx.h"
#include "PlaneSet.h"

X_NAMESPACE_BEGIN(level)

XPlaneSet::XPlaneSet(core::MemoryArenaBase* arena) :
    core::Array<Planef>(g_arena),
    hash_(g_arena)
{
    setGranularity(1024);
    reserve(4096 * 4);
}

int XPlaneSet::FindPlane(const Planef& plane, const float normalEps, const float distEps)
{
    int32_t i, border, hashKey;

    hashKey = static_cast<int>(math<float>::abs(plane.getDistance()) * 0.125f);
    for (border = -1; border <= 1; border++) {
        for (i = hash_.first(hashKey + border); i >= 0; i = hash_.next(i)) {
            if ((*this)[i].compare(plane, normalEps, distEps)) {
                return i;
            }
        }
    }

    PlaneType::Enum type = plane.getType();

    if (type >= PlaneType::NEGX && PlaneType::isTrueAxial(type)) {
        push_back(-plane);
        hash_.add(hashKey, static_cast<int32_t>(size()) - 1);
        push_back(plane);
        hash_.add(hashKey, static_cast<int32_t>(size()) - 1);
        return static_cast<int>(size() - 1);
    }

    push_back(plane);
    hash_.add(hashKey, static_cast<int32_t>(size()) - 1);
    push_back(-plane);
    hash_.add(hashKey, static_cast<int32_t>(size()) - 1);
    return static_cast<int32_t>(size() - 2);
}

X_NAMESPACE_END