#pragma once

#include <Containers\Array.h>
#include <Util\Span.h>

X_NAMESPACE_BEGIN(model)

class KDop
{
public:
    X_DECLARE_ENUM(Type)
    (
        KDOP_10_X,
        KDOP_10_Y,
        KDOP_10_Z,
        KDOP_14,
        KDOP_18,
        KDOP_26);

    struct TriangleInfo
    {
        const Vec3f* pData;
        size_t stride;
        size_t count;
    };

    typedef core::Array<float> FloatArr;
    typedef core::Array<Planef> PlaneArr;

public:
    KDop(Type::Enum type, core::MemoryArenaBase* arena);

    void addTriangles(const TriangleInfo& triInfo);

    void build(void);

    const PlaneArr& getPlanes(void) const;

private:
    core::span<const Vec3f> planeNormals_;

    FloatArr maxDist_;
    PlaneArr planes_;
};

X_NAMESPACE_END