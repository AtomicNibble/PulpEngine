#pragma once

#include "LvlEntity.h"

X_NAMESPACE_BEGIN(level)

class ModelCache;
class MatManager;

struct SourceStats
{
    SourceStats()
    {
        core::zero_this(this);
    }

    size_t numEntities;
    size_t numPatches;
    size_t numBrushes;
};

class LvlSource
{
    typedef core::Array<LvlEntity> LvlEntsArr;

public:
    LvlSource(core::MemoryArenaBase* arena, ModelCache& modelCache, MatManager& matMan, XPlaneSet& planes);
    virtual ~LvlSource();

    void printInfo(void);
    X_INLINE const LvlEntsArr& getEntsArr(void) const;
    X_INLINE LvlEntsArr& getEntsArr(void);
    X_INLINE const AABB& getBounds(void) const;
    X_INLINE const SourceStats& getStats(void) const;

protected:
    void calculateLvlBounds(void);

    X_INLINE int32_t findFloatPlane(const Planef& plane);

protected:
    core::MemoryArenaBase* arena_;
    ModelCache& modelCache_;
    MatManager& matMan_;

    XPlaneSet& planes_;
    LvlEntsArr entities_;
    AABB mapBounds_;

    SourceStats stats_;
};

X_NAMESPACE_END

#include "LvlSource.inl"