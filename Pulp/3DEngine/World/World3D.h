#pragma once

#include <IWorld3D.h>
#include <ILevel.h>

#include "Model\RenderMesh.h"

X_NAMESPACE_DECLARE(model,
                    struct MeshHeader;)

X_NAMESPACE_DECLARE(core,
    namespace V2 {
        struct Job;
        class JobSystem;
    })

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    class Emitter;
} // namespace fx

class IPrimativeContext;
class PrimativeContext;

class DrawVars;

class RenderEnt : public IRenderEnt
{
    template<typename T>
    using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 16>>;

public:
    typedef AlignedArray<Matrix44f> MatrixArr;

public:
    RenderEnt(core::MemoryArenaBase* arena);

    int32_t index;
    int32_t lastModifiedFrameNum; // to determine if it is constantly changing,
    int32_t viewCount;

    model::XModel* pModel;
    MatrixArr bones;

    Transformf trans;

    AABB localBounds;
    AABB globalBounds;
};

class RenderLight : public IRenderLight
{
public:
    RenderLight();

public:
    Transformf trans;

    Colorf col;
};

struct AreaNode
{
    static const int32_t CHILDREN_HAVE_MULTIPLE_AREAS = -2;
    static const int32_t AREANUM_SOLID = -1;

public:
    AreaNode();

public:
    Planef plane;
    // negative numbers are (-1 - areaNumber), 0 = solid
    int32_t children[level::Side::ENUM_COUNT];
    int32_t commonChildrenArea;
};

struct AreaPortal
{
    AreaPortal();
    ~AreaPortal();

public:
    int32_t areaTo;     // the area this portal leads to.
    XWinding* pWinding; // winding points have counter clockwise ordering seen this area
    Planef plane;       // view must be on the positive side of the plane to cross
    core::Array<Vec3f> debugVerts;
};

struct PortalStack
{
    static const size_t MAX_PORTAL_PLANES = 20;
    typedef core::FixedArray<Planef, PortalStack::MAX_PORTAL_PLANES + 1> PortalPlanesArr;

public:
    PortalStack();

public:
    const AreaPortal* pPortal;
    const struct PortalStack* pNext;

    Rectf rect;
    PortalPlanesArr portalPlanes;
};

// data for each portal that can see into the area
struct AreaVisiblePortal
{
    typedef core::FixedArray<Planef, PortalStack::MAX_PORTAL_PLANES + 1> PortalPlanesArr;
    typedef core::Array<uint32_t> EntIdArr;

public:
    AreaVisiblePortal();

public:
    EntIdArr visibleEnts;   // this list of ent's that are visible in the portal planes below.
    PortalPlanesArr planes; // the planes pointing into area
    int32_t areaFrom;       // the area we are coming from
};

struct Area
{
    typedef core::Array<AreaVisiblePortal> VisPortalsArr;
    typedef core::Array<AreaPortal> AreaPortalArr;
    typedef core::Array<uint32_t> EntIdArr;
    typedef core::Array<RenderEnt*> RenderEntPtrArr;

public:
    Area();
    ~Area();

    void destoryRenderMesh(render::IRender* pRender);

    const AABB getBounds(void) const;

public:
    int32_t areaNum;
    int32_t viewCount;
    // points the the area's mesh header.
    model::MeshHeader* pMesh;
    // the gpu buffers for the area mesh.
    model::XRenderMesh renderMesh;
    // the physics for this area.
    physics::ActorHandle physicsActor;
    // portals leading out this area.
    AreaPortalArr portals;

    // info for portals leading into this area from current camers.
    // when this area is visible, this will contain all the portal planes that point in
    // and also a list of visible ents through said portal.
    int32_t curVisPortalIdx;
    int32_t maxVisPortals; // this is the max portals that can point into this area
    VisPortalsArr visPortals;

    // processed vis ents.
    EntIdArr areaVisibleEnts;

    RenderEntPtrArr renderEnts;
};

// these should be like 3d representation of the world.
class World3D : public IWorld3D
{
    typedef core::Array<Area> AreaArr;
    typedef core::Array<Area*> AreaPtrArr;
    typedef core::Array<AreaNode> AreaNodeArr;
    typedef core::Array<level::AreaEntRef> AreaRefsArr;
    typedef core::Array<level::FileAreaRefHdr> AreaRefsHdrArr;
    typedef core::Array<level::MultiAreaEntRef> AreaMultiRefsArr;
    typedef std::array<level::FileAreaRefHdr, level::MAP_MAX_MULTI_REF_LISTS> AreaMultiRefsHdrArr;
    typedef std::array<uint32_t, level::MAP_MAX_MULTI_REF_LISTS> AreaVisFlags;
    typedef core::Array<level::StaticModel> StaticModelsArr;
    typedef core::Array<level::Light> StaticLightssArr;
    typedef core::Array<RenderEnt*> RenderEntPtrArr;
    typedef core::Array<fx::Emitter*> EmitterPtrArr;

    struct AreaRefInfo
    {
        AreaRefInfo(core::MemoryArenaBase* arena);

        void clear(void);
        void free(void);

        // model with single area ref.
        AreaRefsHdrArr areaRefHdrs;
        AreaRefsArr areaRefs;
        // multi area model refrences for models that are in multiple area's
        AreaMultiRefsHdrArr areaMultiRefHdrs;
        AreaMultiRefsArr areaMultiRefs;
    };

public:
    World3D(DrawVars& vars, engine::PrimativeContext* pPrimContex, CBufferManager* pCBufMan,
        physics::IScene* pPhysScene, core::MemoryArenaBase* arena);
    virtual ~World3D() X_FINAL;

    void renderView(core::FrameData& frame, render::CommandBucket<uint32_t>& bucket);
    void renderEmitters(core::FrameData& frame, IPrimativeContext* pContext);

    bool loadNodes(const level::FileHeader& fileHdr, level::StringTable& strTable, uint8_t* pData) X_FINAL;
    IRenderEnt* addRenderEnt(const RenderEntDesc& ent) X_FINAL;
    void freeRenderEnt(IRenderEnt* pEnt) X_FINAL;
    void updateRenderEnt(IRenderEnt* pEnt, const Transformf& trans, bool force) X_FINAL;
    bool setBonesMatrix(IRenderEnt* pEnt, const Matrix44f* pMats, size_t num) X_FINAL;

    IRenderLight* addRenderLight(const RenderLightDesc& ent) X_FINAL;

    fx::IEmitter* addEmmiter(const EmitterDesc& emit) X_FINAL;
    void freeEmitter(fx::IEmitter* pEmitter) X_FINAL;

    // util
    X_INLINE size_t numAreas(void) const;
    X_INLINE size_t numPortalsInArea(int32_t areaNum) const;
    X_INLINE bool areaHasPortals(int32_t areaNum) const;

    X_INLINE bool isPointInAnyArea(const Vec3f& pos) const;
    bool isPointInAnyArea(const Vec3f& pos, int32_t& areaOut) const;

    size_t boundsInAreas(const AABB& bounds, int32_t* pAreasOut, size_t maxAreas) const;

    X_INLINE bool isCamArea(int32_t areaNum) const;
    X_INLINE bool isAreaVisible(int32_t areaNum) const;
    X_INLINE bool isAreaVisible(const Area& area) const;

private:
    void boundsInAreas_r(int32_t nodeNum, const AABB& bounds, size_t& numAreasOut,
        int32_t* pAreasOut, size_t maxAreas) const;

    void pushFrustumIntoTree_r(RenderEnt* pEnt, int32_t nodeNum);
    void pushFrustumIntoTree(RenderEnt* pEnt);

    void addEntityToArea(Area& area, RenderEnt* pEnt);

    int32_t commonChildrenArea_r(AreaNode* pAreaNode);

private:
    void clearVisPortals(void);
    void findVisibleArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void floodThroughPortal_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void floodViewThroughArea_r(core::V2::Job* pParentJob, const Vec3f origin, int32_t areaNum, int32_t areaFrom, const Planef& farPlane,
        const PortalStack* ps);

    void setAreaVisible(int32_t area);
    void setAreaVisible(int32_t area, int32_t areaFrom, const PortalStack* ps = nullptr);
    void setAreaVisibleAndCull(core::V2::Job* pParentJob, int32_t area, int32_t areaFrom, const PortalStack* ps = nullptr);

private:
    void cullArea_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void buildVisibleAreaFlags_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void mergeVisibilityArrs_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

    void drawVisibleAreaGeo_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void drawVisibleStaticModels_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

    void drawAreaGeo(Area** pAreas, uint32_t num);
    void drawStaticModels(const uint32_t* pModelIds, uint32_t num);
    void drawRenderEnts();

    void addMeshTobucket(const model::MeshHeader& mesh, const model::XRenderMesh& renderMesh, render::shader::VertexFormat::Enum vrtvertFmtFmt,
        const Matrix44f& world, const float distanceFromCam, render::VertexBufferHandle boneData);

    void addMeshTobucket(const model::MeshHeader& mesh, const model::XRenderMesh& renderMesh, render::shader::VertexFormat::Enum vrtvertFmtFmt, const Matrix44f& world, const float distanceFromCam);

private:
    void createEntityRefs(RenderEnt* pEnt);

private:
    void drawDebug(void);
    void debugDraw_AreaBounds(void) const;
    void debugDraw_Portals(void) const;
    void debugDraw_PortalStacks(void) const;
    void debugDraw_Lights(void) const;
    void debugDraw_StaticModelCullVis(void) const;
    void debugDraw_ModelBones(void) const;
    void debugDraw_DrawDetachedCam(void) const;

private:
    render::CommandBucket<uint32_t>* pBucket_;

private:
    core::MemoryArenaBase* arena_;
    physics::IScene* pPhysScene_;
    engine::PrimativeContext* pPrimContex_;
    CBufferManager* pCBufMan_;

    DrawVars& vars_;

    bool outsideWorld_;

    int32_t viewCount_;
    int32_t frameNumber_;

    int32_t camArea_;
    XCamera cam_;

    AreaArr areas_;
    AreaNodeArr areaNodes_;

    AreaRefInfo modelRefs_;

    // lights
    StaticLightssArr lights_;

    // all static models
    StaticModelsArr staticModels_;

    // cleared each frame.
    AreaVisFlags visibleAreaFlags_;

    core::Spinlock visAreaLock_;
    AreaPtrArr visibleAreas_;

private:
    RenderEntPtrArr renderEnts_;
    EmitterPtrArr emitters_;
};

X_NAMESPACE_END

#include "World3D.inl"