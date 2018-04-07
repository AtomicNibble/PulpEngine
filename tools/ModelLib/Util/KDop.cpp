#include "stdafx.h"
#include "KDop.h"

X_NAMESPACE_BEGIN(model)

namespace
{
    const Vec3f KDopDir10X[10] = {
        Vec3f(1.f, 0.f, 0.f),
        Vec3f(-1.f, 0.f, 0.f),
        Vec3f(0.f, 1.f, 0.f),
        Vec3f(0.f, -1.f, 0.f),
        Vec3f(0.f, 0.f, 1.f),
        Vec3f(0.f, 0.f, -1.f),
        Vec3f(0.f, math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2)};

    const Vec3f KDopDir10Y[10] = {
        Vec3f(1.f, 0.f, 0.f),
        Vec3f(-1.f, 0.f, 0.f),
        Vec3f(0.f, 1.f, 0.f),
        Vec3f(0.f, -1.f, 0.f),
        Vec3f(0.f, 0.f, 1.f),
        Vec3f(0.f, 0.f, -1.f),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2)};

    const Vec3f KDopDir10Z[10] = {
        Vec3f(1.f, 0.f, 0.f),
        Vec3f(-1.f, 0.f, 0.f),
        Vec3f(0.f, 1.f, 0.f),
        Vec3f(0.f, -1.f, 0.f),
        Vec3f(0.f, 0.f, 1.f),
        Vec3f(0.f, 0.f, -1.f),
        Vec3f(math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f)};

    // shiiet.
    const Vec3f KDopDir14[18] = {
        Vec3f(1.f, 0.f, 0.f),
        Vec3f(-1.f, 0.f, 0.f),
        Vec3f(0.f, 1.f, 0.f),
        Vec3f(0.f, -1.f, 0.f),
        Vec3f(0.f, 0.f, 1.f),
        Vec3f(0.f, 0.f, -1.f),
        Vec3f(0.f, math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f)};

    const Vec3f KDopDir18[18] = {
        Vec3f(1.f, 0.f, 0.f),
        Vec3f(-1.f, 0.f, 0.f),
        Vec3f(0.f, 1.f, 0.f),
        Vec3f(0.f, -1.f, 0.f),
        Vec3f(0.f, 0.f, 1.f),
        Vec3f(0.f, 0.f, -1.f),
        Vec3f(0.f, math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f)};

    const Vec3f KDopDir26[26] = {
        Vec3f(1.f, 0.f, 0.f),
        Vec3f(-1.f, 0.f, 0.f),
        Vec3f(0.f, 1.f, 0.f),
        Vec3f(0.f, -1.f, 0.f),
        Vec3f(0.f, 0.f, 1.f),
        Vec3f(0.f, 0.f, -1.f),
        Vec3f(0.f, math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2),
        Vec3f(0.f, -math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, 0.f, -math<float>::SQRT_1OVER2),
        Vec3f(-math<float>::SQRT_1OVER2, 0.f, math<float>::SQRT_1OVER2),
        Vec3f(math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(math<float>::SQRT_1OVER2, -math<float>::SQRT_1OVER2, 0.f),
        Vec3f(-math<float>::SQRT_1OVER2, math<float>::SQRT_1OVER2, 0.f),
        Vec3f(math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3),
        Vec3f(math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3),
        Vec3f(math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3),
        Vec3f(math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3),
        Vec3f(-math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3),
        Vec3f(-math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3),
        Vec3f(-math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3, math<float>::SQRT_1OVER3),
        Vec3f(-math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3, -math<float>::SQRT_1OVER3),
    };

} // namespace

KDop::KDop(Type::Enum type, core::MemoryArenaBase* arena) :
    maxDist_(arena),
    planes_(arena)
{
    switch (type) {
        case Potato::model::KDop::Type::KDOP_10_X:
            planeNormals_ = core::make_span<const Vec3f>(KDopDir10X);
            break;
        case Potato::model::KDop::Type::KDOP_10_Y:
            planeNormals_ = core::make_span<const Vec3f>(KDopDir10Y);
            break;
        case Potato::model::KDop::Type::KDOP_10_Z:
            planeNormals_ = core::make_span<const Vec3f>(KDopDir10Z);
            break;
        case Potato::model::KDop::Type::KDOP_14:
            planeNormals_ = core::make_span<const Vec3f>(KDopDir18);
            break;
        case Potato::model::KDop::Type::KDOP_18:
            planeNormals_ = core::make_span<const Vec3f>(KDopDir18);
            break;
        case Potato::model::KDop::Type::KDOP_26:
            planeNormals_ = core::make_span<const Vec3f>(KDopDir26);
            break;
        default:
            X_ASSERT_UNREACHABLE();
            break;
    }

    planes_.resize(planeNormals_.size());
    maxDist_.resize(planeNormals_.size());
    std::fill(maxDist_.begin(), maxDist_.end(), std::numeric_limits<float>::min());
}

void KDop::addTriangles(const TriangleInfo& triInfo)
{
    X_ASSERT(!planeNormals_.empty(), "No plane directions")(); 
    X_ASSERT(maxDist_.size() == planeNormals_.size(), "Size mismatch")(); 

    const Vec3f* pVert = triInfo.pData;
    const auto stride = triInfo.stride;

    for (size_t i = 0; i < triInfo.count; i++) {
        const auto& vert = *pVert;

        for (size_t j = 0; j < maxDist_.size(); j++) {
            const auto& dir = planeNormals_[j];
            float dist = dir.dot(vert);

            maxDist_[j] = core::Max(maxDist_[j], dist);
        }

        pVert = reinterpret_cast<const Vec3f*>(reinterpret_cast<const uint8_t*>(pVert) + stride);
    }
}

void KDop::build(void)
{
    // infate a bit.
    //for (auto& d : maxDist_) {
        // d += 0.1f;
    //}

    for (size_t i = 0; i < maxDist_.size(); i++) {
        planes_[i] = Planef(planeNormals_[i], maxDist_[i]);
    }
}

const KDop::PlaneArr& KDop::getPlanes(void) const
{
    return planes_;
}

X_NAMESPACE_END
