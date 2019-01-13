#include "stdafx.h"
#include "Effect.h"

#include "EngineEnv.h"
#include "Material\MaterialManager.h"

#include <IEffect.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    Effect::Effect(core::string_view name, core::MemoryArenaBase* arena) :
        core::AssetBase(name, assetDb::AssetType::FX),
        numStages_(0),
        numIndex_(0),
        numFloats_(0),
        dataSize_(0)
    {
        X_UNUSED(arena);
    }

    Effect::~Effect()
    {
    }

    bool Effect::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        if (dataSize < sizeof(EffectHdr)) {
            return false;
        }

        EffectHdr& hdr = *reinterpret_cast<EffectHdr*>(data.get());

        if (!hdr.isValid()) {
            X_ERROR("Fx", "\"%s\" invalid header", name_.c_str());
            return false;
        }

        numStages_ = hdr.numStages;
        numIndex_ = hdr.numIndex;
        numFloats_ = hdr.numFloats;

        dataSize_ = dataSize;
        data_ = std::move(data);

        for (int32_t i = 0; i < getNumStages(); i++)
        {
            auto& stage = getStageDsc(i);

            const char* pMtlName = getMaterialName(stage.materialStrOffset);

            gEngEnv.pMaterialMan_->loadMaterial(core::string_view(pMtlName));
        }

        return true;
    }

    const StageDsc& Effect::getStageDsc(int32_t idx) const
    {
        X_ASSERT(idx >= 0 && idx < getNumStages(), "Out of bouds")(idx, getNumStages());
        return getStageDscs()[idx];
    }

    const StageDsc* Effect::getStageDscs(void) const
    {
        return reinterpret_cast<StageDsc*>(data_.ptr() + sizeof(EffectHdr));
    }

    const IndexType* Effect::getIndexes(void) const
    {
        size_t offset = sizeof(EffectHdr);
        offset += sizeof(StageDsc) * numStages_;

        return reinterpret_cast<IndexType*>(data_.ptr() + offset);
    }

    const float* Effect::getFloats(void) const
    {
        size_t offset = sizeof(EffectHdr);
        offset += sizeof(StageDsc) * numStages_;
        offset += sizeof(IndexType) * numIndex_;

        return reinterpret_cast<float*>(data_.ptr() + offset);
    }

    const char* Effect::getMaterialName(int32_t strOffset) const
    {
        size_t offset = sizeof(EffectHdr);
        offset += sizeof(StageDsc) * numStages_;
        offset += sizeof(IndexType) * numIndex_;
        offset += sizeof(float) * numFloats_;
        offset += strOffset;

        return reinterpret_cast<const char*>(data_.ptr() + offset);
    }

    float Effect::fromGraph(const Graph& g, float t) const
    {
        if (g.numPoints == 0) {
            return 0.f;
        }

        float scale = getFloat(g.scaleIdx);
        float result = 0.f;

        if (g.numPoints > 1) {
            for (int32_t i = 0; i < g.numPoints; i++) {
                auto val0 = floatForIdx(g.timeStart + i);

                if (val0 == t) {
                    result = floatForIdx(g.valueStart + i);
                    break;
                }
                else if (val0 > t) {
                    // blend.
                    val0 = floatForIdx(g.timeStart + (i - 1));
                    auto val1 = floatForIdx(g.timeStart + i);

                    auto res0 = floatForIdx(g.valueStart + (i - 1));
                    auto res1 = floatForIdx(g.valueStart + i);

                    float offset = t - val0;
                    float range = val1 - val0;
                    float fraction = offset / range;

                    result = lerp(res0, res1, fraction);
                    break;
                }
            }
        }
        else {
            result = floatForIdx(g.valueStart);
        }

        return result * scale;
    }

    Vec3f Effect::fromColorGraph(const Graph& g, float t) const
    {
        X_ASSERT(g.numPoints > 0, "Hraph is empty")(g.numPoints);

        float scale = getFloat(g.scaleIdx);
        Vec3f result;

        if (g.numPoints > 1) {
            for (int32_t i = 0; i < g.numPoints; i++) {
                auto val0 = floatForIdx(g.timeStart + i);

                if (val0 == t) {
                    result = colorForIdx(g.valueStart, i);
                    break;
                }
                else if (val0 > t) {
                    // blend.
                    val0 = floatForIdx(g.timeStart + (i - 1));
                    auto val1 = floatForIdx(g.timeStart + i);

                    auto res0 = colorForIdx(g.valueStart, i - 1);
                    auto res1 = colorForIdx(g.valueStart, i);

                    float offset = t - val0;
                    float range = val1 - val0;
                    float fraction = offset / range;

                    result = res0.lerp(fraction, res1);
                    break;
                }
            }
        }
        else {
            result = colorForIdx(g.valueStart, 0);
        }

        return result * scale;
    }

    X_INLINE Vec3f Effect::colorForIdx(int32_t start, int32_t idx) const
    {
        idx = start + (idx * 3);
        return Vec3f(
            floatForIdx(idx),
            floatForIdx(idx + 1),
            floatForIdx(idx + 2));
    }

} // namespace fx

X_NAMESPACE_END