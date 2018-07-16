#pragma once

#include "Util\GrowingPool.h"
#include <Memory\SimpleMemoryArena.h>
#include <Memory\AllocationPolicies\GrowingGenericAllocator.h>

#include "LvlEntity.h"
#include "LvlArea.h"

X_NAMESPACE_DECLARE(assetDb, class AssetDB)

X_NAMESPACE_BEGIN(level)

class ModelCache;
class MatManager;

class Compiler
{
#if X_ENABLE_MEMORY_DEBUG_POLICIES

    typedef core::MemoryArena<
        core::GrowingGenericAllocator,
        core::SingleThreadPolicy,
        core::SimpleBoundsChecking,
        // core::SimpleMemoryTracking,
        core::NoMemoryTracking,
        core::SimpleMemoryTagging>
        WindingDataArena;

    typedef core::MemoryArena<
        core::GrowingPoolAllocator,
        core::SingleThreadPolicy,
        core::SimpleBoundsChecking,
        // core::SimpleMemoryTracking,
        core::NoMemoryTracking,
        core::SimpleMemoryTagging>
        PoolArena;
#else

    typedef core::SimpleMemoryArena<
        core::GrowingGenericAllocator>
        WindingDataArena;

    typedef core::SimpleMemoryArena<
        core::GrowingPoolAllocator>
        PoolArena;

#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING

    template<typename T>
    using ArrayExpGrow = core::Array<T, core::ArrayAllocator<T>, core::growStrat::Multiply>;

    typedef core::Array<int32_t> AreaIdArr;
    typedef core::Array<LvlEntity> LvlEntsArr;
    typedef core::Array<LvlArea> LvlAreaArr;
    typedef ArrayExpGrow<level::FileStaticModel> StaticModelsArr;
    typedef ArrayExpGrow<level::Light> LightsArr;
    typedef std::array<core::Array<level::MultiAreaEntRef>,
        level::MAP_MAX_MULTI_REF_LISTS>
        MultiRefArr;

public:
    Compiler(core::MemoryArenaBase* arena, assetDb::AssetDB& db, physics::IPhysicsCooking* pPhysCooking);
    ~Compiler();

    bool init(void);
    bool compileLevel(core::Path<char>& path, core::Path<char>& outPath);

private:
    bool save(const LvlEntsArr& ent, core::Path<char>& path);
    bool processModels(LvlEntsArr& ents);
    bool processModel(LvlEntity& ent);
    bool processWorldModel(LvlEntsArr& ents, LvlEntity& ent);

    bool createAreasForPrimativates(LvlEntity& ent);
    void putWindingIntoAreas_r(Winding* pWinding, LvlBrushSide& side, bspNode* pNode);
    void areasForBounds_r(AreaIdArr& areaList, const Sphere& sphere,
        const Vec3f boundsPoints[8], bspNode* pNode);

    bool createLightList(LvlEntsArr& ents);
    bool createStaticModelList(LvlEntity& ent, LvlEntsArr& ents);
    bool createCollisionData(LvlEntity& ent);

private:
    core::MemoryArenaBase* arena_;
    assetDb::AssetDB& db_;
    physics::IPhysicsCooking* pPhysCooking_;
    ModelCache* pModelCache_;
    MatManager* pMaterialMan_;

    GrowingPool<PoolArena> bspFaceAllocator_;
    GrowingPool<PoolArena> bspPortalAllocator_;
    GrowingPool<PoolArena> bspNodeAllocator_;

    core::GrowingGenericAllocator windingDataAllocator_;
    WindingDataArena windingDataArena_;

    XPlaneSet planes_;

    // Compiled data
    LvlAreaArr areas_;

    LightsArr lights_;

    StaticModelsArr staticModels_;
    MultiRefArr multiModelRefLists_;

    StringTableUnique stringTable_;
};

X_NAMESPACE_END