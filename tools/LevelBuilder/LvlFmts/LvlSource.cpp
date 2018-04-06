#include "stdafx.h"
#include "LvlSource.h"

X_NAMESPACE_BEGIN(level)

LvlSource::LvlSource(core::MemoryArenaBase* arena, ModelCache& modelCache, MatManager& matMan, XPlaneSet& planes) :
    arena_(arena),
    modelCache_(modelCache),
    matMan_(matMan),
    entities_(arena),
    planes_(planes)
{
}

LvlSource::~LvlSource()
{
}

void LvlSource::calculateLvlBounds(void)
{
    mapBounds_.clear();

    // bound me baby
    for (const auto& ent : entities_) {
        for (const auto& brush : ent.brushes) {
            mapBounds_.add(brush.bounds);
        }
    }
}

void LvlSource::printInfo(void)
{
    AABB::StrBuf boundsStr;
    if (entities_.isNotEmpty()) {
        X_LOG0("Map", "Total world brush: ^8%" PRIuS, entities_[0].brushes.size());
    }
    X_LOG0("Map", "Total brush: ^8%" PRIi32, stats_.numBrushes);
    X_LOG0("Map", "Total patches: ^8%" PRIi32, stats_.numPatches);
    X_LOG0("Map", "Total entities: ^8%" PRIi32, stats_.numEntities);
    X_LOG0("Map", "Total planes: ^8%" PRIuS, planes_.size());
    X_LOG0("Map", "Bounds: ^6%s", mapBounds_.toString(boundsStr));
}

X_NAMESPACE_END