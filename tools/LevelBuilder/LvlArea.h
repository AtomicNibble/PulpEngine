#pragma once

#include <String\GrowingStringTable.h>

X_NAMESPACE_BEGIN(level)

struct LvlBrushSide;
struct LvlBrush;

class ColMeshData
{
public:
    typedef core::Array<uint8_t> DataArr;
    typedef core::Array<Vec3f> VertArr;
    typedef core::Array<model::Face> FacesArr;

protected:
    ColMeshData(core::MemoryArenaBase* arena);

public:
    const DataArr& cookedData(void) const;

protected:
    VertArr verts_;
    DataArr cookedData_;
};

// Data for a single TriangleMesh. it's one mesh once cooked.
class ColTriMeshData : public ColMeshData
{
public:
    ColTriMeshData(core::MemoryArenaBase* arena);

    bool cook(physics::IPhysicsCooking* pCooking);

    void addBrush(const LvlBrush& brush);

private:
    FacesArr faces_;
};

class ColConvexMeshData : public ColMeshData
{
public:
    typedef core::Array<model::Index> IndexArr;

public:
    ColConvexMeshData(core::MemoryArenaBase* arena);

    bool cook(physics::IPhysicsCooking* pCooking);

    void addBrush(const LvlBrush& brush);

private:
    IndexArr indexes_;
};

// a collision group bucket, stores all the physics data for a given physics group.
// so for a area we may have multiple collision buckets.
struct ColGroupBucket
{
    typedef core::Array<ColTriMeshData> ColTriMesgDataArr;
    typedef core::Array<ColConvexMeshData> ColConvexMeshDataArr;
    typedef core::Array<AABB> AABBArr;

public:
    ColGroupBucket(physics::GroupFlags groupFlags, core::MemoryArenaBase* arena);

    bool cook(physics::IPhysicsCooking* pCooking);

    X_INLINE void addAABB(const AABB& bounds)
    {
        aabbData_.append(bounds);
    }

    X_INLINE void addTriMesh(ColTriMeshData&& triMesh)
    {
        triMeshData_.append(std::move(triMesh));
    }
    X_INLINE void addConvexMesh(ColConvexMeshData&& convexMesh)
    {
        convexMeshData_.append(std::move(convexMesh));
    }

    physics::GroupFlags getGroupFlags(void) const;
    const ColTriMesgDataArr& getTriMeshDataArr(void) const;
    const ColConvexMeshDataArr& getConvexMeshDataArr(void) const;
    const AABBArr& getAABBData(void) const;

private:
    physics::GroupFlags groupFlags_;

    ColTriMesgDataArr triMeshData_;
    ColConvexMeshDataArr convexMeshData_;
    AABBArr aabbData_;
};

// contains all the collision data for a area.
struct AreaCollsiion
{
    typedef core::Array<uint8_t> DataArr;
    typedef core::Array<ColGroupBucket> ColGroupBucketArr;

public:
    AreaCollsiion(core::MemoryArenaBase* arena);

    bool cook(physics::IPhysicsCooking* pCooking);

    size_t numGroups(void) const;
    const ColGroupBucketArr& getGroups(void) const;

    ColGroupBucket& getBucket(physics::GroupFlags flags);

private:
    ColGroupBucketArr colGroupBuckets_;
};

struct AreaModel
{
    typedef core::Array<model::SubMeshHeader> MeshArr;
    typedef core::Array<level::Vertex> VertArr;
    typedef core::Array<model::Face> FacesArr;

public:
    AreaModel();

    void writeToStream(core::ByteStream& stream) const;
    size_t serializeSize(void) const;

    bool belowLimits(void);
    void beginModel(void);
    void endModel(void);

    MeshArr meshes;
    VertArr verts;
    FacesArr faces;

    model::MeshHeader meshHeader;
};

// used to build up submeshes.
// so faces with same materials are grouped into meshes.
struct AreaSubMesh
{
    typedef core::Array<level::Vertex> VertArr;
    typedef core::Array<model::Face> FacesArr;

public:
    AreaSubMesh();

    X_INLINE void addVert(const level::Vertex& vert)
    {
        verts_.append(vert);
    }
    X_INLINE void addFace(const model::Face& face)
    {
        faces_.append(face);
    }

public:
    MaterialName matName_;
    uint32_t matNameID_;

    // index's for this sub mesh.
    // merged into AreaModel list at end.
    VertArr verts_;
    FacesArr faces_;
};

class LvlArea
{
    typedef core::HashMap<core::string, AreaSubMesh> AreaMeshMap;
    typedef core::Array<uint32_t> AreaRefs;

public:
    LvlArea();

    void areaBegin(void);
    bool areaEnd(StringTableUnique& stringTable);

    void addWindingForSide(const XPlaneSet& planes, const LvlBrushSide& side, Winding* pWinding);

    AreaSubMesh* meshForSide(const LvlBrushSide& side, StringTableUnique& stringTable);
    AreaSubMesh* meshForMat(const core::string& matName, StringTableUnique& stringTable);

private:
    AreaSubMesh* meshForSide(const LvlBrushSide& side);

public:
    // area has one model.
    AreaModel model;
    AreaCollsiion collision;

    AreaMeshMap areaMeshes;
    AreaRefs entRefs;
    AreaRefs modelsRefs;

    // copy of the model values.
    AABB boundingBox;
    Sphere boundingSphere;
};

X_NAMESPACE_END
