

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    X_INLINE float Effect::getFloat(int32_t idx) const
    {
        auto* pFlts = getFloats();

        return pFlts[idx];
    }

    X_INLINE float Effect::floatForIdx(int32_t idx) const
    {
        auto* pIndexes = getIndexes();
        auto* pFlts = getFloats();

        idx = pIndexes[idx];
        return pFlts[idx];
    }

    X_INLINE int32_t Effect::getNumStages(void) const
    {
        return numStages_;
    }

} // namespace fx

X_NAMESPACE_END