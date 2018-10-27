#include "stdafx.h"
#include "LvlArea.h"
#include "LvlEntity.h"

X_NAMESPACE_BEGIN(level)

ColMeshData::ColMeshData(core::MemoryArenaBase* arena) :
    verts_(arena),
    cookedData_(arena)
{
}

const ColMeshData::DataArr& ColMeshData::cookedData(void) const
{
    return cookedData_;
}

// ==========================================

ColTriMeshData::ColTriMeshData(core::MemoryArenaBase* arena) :
    ColMeshData(arena),
    faces_(arena)
{
}

void ColTriMeshData::addBrush(const LvlBrush& brush)
{
    for (auto& side : brush.sides) {
        auto* pWinding = side.pWinding;
        const size_t numPoints = pWinding->getNumPoints();

        for (size_t i = 2; i < numPoints; i++) {
            uint16_t facesIdx[3];

            for (size_t j = 0; j < 3; j++) {
                Vec3f pos;

                if (j == 0) {
                    const Vec5f vec = pWinding->at(0);
                    pos = vec.asVec3();
                }
                else if (j == 1) {
                    const Vec5f vec = pWinding->at(i - 1);
                    pos = vec.asVec3();
                }
                else {
                    const Vec5f vec = pWinding->at(i);
                    pos = vec.asVec3();
                }

                size_t v;
                for (v = 0; v < verts_.size(); v++) {
                    if (verts_[v].compare(pos, 0.1f)) {
                        facesIdx[j] = safe_static_cast<model::Index>(v);
                        break;
                    }
                }

                if (v == verts_.size()) {
                    verts_.append(pos);
                    facesIdx[j] = safe_static_cast<model::Index>(v);
                }
            }

            faces_.emplace_back(facesIdx[0], facesIdx[1], facesIdx[2]);
        }
    }
}

bool ColTriMeshData::cook(physics::IPhysicsCooking* pCooking)
{
    using namespace physics;

    IPhysicsCooking::CookFlags flags;

    static_assert(sizeof(decltype(faces_)::Type::value_type) == 2, "No longer using 16bit indicies?");
    flags.Set(IPhysicsCooking::CookFlag::INDICES_16BIT);

    TriangleMeshDesc desc;
    desc.points.pData = verts_.data();
    desc.points.stride = sizeof(decltype(verts_)::Type);
    desc.points.count = safe_static_cast<uint32>(verts_.size());
    desc.triangles.pData = faces_.data();
    desc.triangles.stride = sizeof(decltype(faces_)::Type);
    desc.triangles.count = safe_static_cast<uint32>(faces_.size());

    if (!pCooking->cookTriangleMesh(desc, cookedData_, flags)) {
        X_ERROR("TriMesh", "Failed to cook area collision");
        return false;
    }

    if (cookedData_.size() > level::MAP_MAX_AREA_COL_DATA_SIZE) {
        X_ERROR("TriMesh", "cooked mesh is too big: %" PRIuS " bytes. max: %" PRIu32, cookedData_.size(), level::MAP_MAX_AREA_COL_DATA_SIZE);
        return false;
    }

    return true;
}

// ==========================================

ColConvexMeshData::ColConvexMeshData(core::MemoryArenaBase* arena) :
    ColMeshData(arena),
    indexes_(arena)
{
}

void ColConvexMeshData::addBrush(const LvlBrush& brush)
{
    for (auto& side : brush.sides) {
        auto* pWinding = side.pWinding;
        const size_t numPoints = pWinding->getNumPoints();

        for (size_t i = 0; i < numPoints; i++) {
            verts_.append(pWinding->at(i).asVec3());
        }
    }
}

bool ColConvexMeshData::cook(physics::IPhysicsCooking* pCooking)
{
    using namespace physics;

    IPhysicsCooking::CookFlags flags;

    static_assert(sizeof(decltype(indexes_)::Type) == 2, "No longer using 16bit indicies?");
    flags.Set(IPhysicsCooking::CookFlag::INDICES_16BIT);
    flags.Set(IPhysicsCooking::CookFlag::COMPUTE_CONVEX);

    ConvexMeshDesc desc;
    desc.points.pData = verts_.data();
    desc.points.stride = sizeof(decltype(verts_)::Type);
    desc.points.count = safe_static_cast<uint32>(verts_.size());

    if (!pCooking->cookConvexMesh(desc, cookedData_, flags)) {
        X_ERROR("ConvexMesh", "Failed to cook collision mesh");
        return false;
    }

    if (cookedData_.size() > level::MAP_MAX_AREA_COL_DATA_SIZE) {
        X_ERROR("ConvexMesh", "Cooked mesh is too big: %" PRIuS " bytes. max: %" PRIu32, cookedData_.size(), level::MAP_MAX_AREA_COL_DATA_SIZE);
        return false;
    }

    return true;
}

// ==========================================

ColGroupBucket::ColGroupBucket(physics::GroupFlags groupFlags, core::MemoryArenaBase* arena) :
    groupFlags_(groupFlags),
    triMeshData_(arena),
    convexMeshData_(arena),
    aabbData_(arena)
{
    aabbData_.setGranularity(128);
}

bool ColGroupBucket::cook(physics::IPhysicsCooking* pCooking)
{
    for (auto& tri : triMeshData_) {
        if (!tri.cook(pCooking)) {
            return false;
        }
    }

    return true;
}

physics::GroupFlags ColGroupBucket::getGroupFlags(void) const
{
    return groupFlags_;
}

const ColGroupBucket::ColTriMesgDataArr& ColGroupBucket::getTriMeshDataArr(void) const
{
    return triMeshData_;
}

const ColGroupBucket::ColConvexMeshDataArr& ColGroupBucket::getConvexMeshDataArr(void) const
{
    return convexMeshData_;
}

const ColGroupBucket::AABBArr& ColGroupBucket::getAABBData(void) const
{
    return aabbData_;
}

// ==========================================

AreaCollsiion::AreaCollsiion(core::MemoryArenaBase* arena) :
    colGroupBuckets_(arena)
{
    colGroupBuckets_.setGranularity(4);
}

bool AreaCollsiion::cook(physics::IPhysicsCooking* pCooking)
{
    for (auto& b : colGroupBuckets_) {
        if (!b.cook(pCooking)) {
            return false;
        }
    }

    return true;
}

size_t AreaCollsiion::numGroups(void) const
{
    return colGroupBuckets_.size();
}

const AreaCollsiion::ColGroupBucketArr& AreaCollsiion::getGroups(void) const
{
    return colGroupBuckets_;
}

ColGroupBucket& AreaCollsiion::getBucket(physics::GroupFlags flags)
{
    for (auto& b : colGroupBuckets_) {
        if (b.getGroupFlags() == flags) {
            return b;
        }
    }

    // add a new one :O
    colGroupBuckets_.emplace_back(flags, colGroupBuckets_.getArena());

    return colGroupBuckets_.back();
}

// ----------------------------

AreaModel::AreaModel() :
    meshes(g_arena),
    verts(g_arena),
    faces(g_arena)
{
}

void AreaModel::writeToStream(core::ByteStream& stream) const
{
    stream.reserve(stream.size() + serializeSize());

    stream.write(meshHeader);
    stream.write(meshes.ptr(), meshes.size());

    stream.alignWrite(16);
    for (const auto& vert : verts) {
        stream.write(vert.pos);
        stream.write(vert.texcoord[0]);
        stream.write(vert.texcoord[1]);
    }

    stream.alignWrite(16);
    for (const auto& vert : verts) {
        stream.write(vert.color);
    }

    stream.alignWrite(16);
    for (const auto& vert : verts) {
        stream.write(vert.normal);
    }

    stream.alignWrite(16);
    stream.write(faces.ptr(), faces.size());
}

size_t AreaModel::serializeSize(void) const
{
    size_t numBytes = 0;
    numBytes += sizeof(meshHeader);
    numBytes += core::bitUtil::RoundUpToMultiple(sizeof(model::SubMeshHeader) * meshes.size(), 16_sz);
    numBytes += core::bitUtil::RoundUpToMultiple(sizeof(level::Vertex) * verts.size(), 16_sz);
    numBytes += core::bitUtil::RoundUpToMultiple(sizeof(model::Face) * faces.size(), 16_sz);
    return numBytes;
}

bool AreaModel::belowLimits(void)
{
    if (meshes.size() > level::MAP_MAX_MODEL_SURFACES) {
        X_ERROR("AreaModel", "too many surfaces on AreaModel. num: %i max: %i",
            meshes.size(), level::MAP_MAX_MODEL_SURFACES);
        return false;
    }
    if (verts.size() > level::MAP_MAX_MODEL_VERTS) {
        X_ERROR("AreaModel", "too many verts for AreaModel. num: %i max: %i",
            verts.size(), level::MAP_MAX_MODEL_VERTS);
        return false;
    }
    if ((faces.size() * 3) > level::MAP_MAX_MODEL_INDEXES) {
        X_ERROR("AreaModel", "too many indexes for AreaModel. num: %i max: %i",
            faces.size() * 3, level::MAP_MAX_MODEL_INDEXES);
        return false;
    }

    return true;
}

void AreaModel::beginModel(void)
{
    meshes.setGranularity(4096);
    verts.setGranularity(4096);
    faces.setGranularity(4096);
}

void AreaModel::endModel(void)
{
    meshHeader.streamsFlag = model::StreamType::NORMALS | model::StreamType::COLOR;
    meshHeader.numSubMeshes = safe_static_cast<uint32_t, size_t>(meshes.size());
    meshHeader.numVerts = safe_static_cast<uint32_t, size_t>(verts.size());
    meshHeader.numFaces = safe_static_cast<uint32_t, size_t>(faces.size());

    // build bounds for all the meshes.
    // this could be done in parrell.
    AABB bounds;

    bounds.clear();

    auto it = meshes.begin();
    for (; it != meshes.end(); ++it) {
        bounds.add(it->boundingBox);
    }

    meshHeader.boundingBox = bounds;
    meshHeader.boundingSphere = Sphere(bounds);

    AABB::StrBuf buf;
    X_LOG_BULLET;
    X_LOG1("AreaModel", "num verts: %i", meshHeader.numVerts);
    X_LOG1("AreaModel", "num Faces: %i", meshHeader.numFaces);
    X_LOG1("AreaModel", "num meshes: %i", meshHeader.numSubMeshes);
    X_LOG1("AreaModel", "bounds: %s", bounds.toString(buf));
}

// ----------------------------

AreaSubMesh::AreaSubMesh() :
    verts_(g_arena),
    faces_(g_arena)
{
    verts_.setGranularity(4096);
    faces_.setGranularity(4096);
}

// ----------------------------

LvlArea::LvlArea() :
    collision(g_arena),
    areaMeshes(g_arena),
    entRefs(g_arena),
    modelsRefs(g_arena)
{
    areaMeshes.reserve(2048);
    entRefs.reserve(256);
}

void LvlArea::areaBegin(void)
{
    areaMeshes.clear();

    model.beginModel();
}

bool LvlArea::areaEnd(StringTableUnique& stringTable)
{
    if (areaMeshes.empty()) {
        X_ERROR("LvlArea", "Area has no meshes");
        return false;
    }

    for (const auto& it : areaMeshes) {
        const auto& subMesh = it.second;

        model::SubMeshHeader mesh;
        mesh.boundingBox.clear();

        for (const auto& vert : subMesh.verts_) {
            mesh.boundingBox.add(vert.pos);
        }

        mesh.boundingSphere = Sphere(mesh.boundingBox);
        mesh.numFaces = safe_static_cast<uint16_t, size_t>(subMesh.faces_.size());
        mesh.numVerts = safe_static_cast<uint16_t, size_t>(subMesh.verts_.size());
        mesh.startIndex = safe_static_cast<uint32_t, size_t>(model.faces.size() * 3);
        mesh.startVertex = safe_static_cast<uint32_t, size_t>(model.verts.size());
        mesh.streamsFlag = model::StreamType::COLOR | model::StreamType::NORMALS;

        auto matNameID = stringTable.addStringUnqiue(subMesh.matName_.c_str(), subMesh.matName_.length());

        mesh.materialName = matNameID;

        X_LOG1("SubMesh", "Mat: ^3%s^7 verts: %i faces: %i",
            subMesh.matName_.c_str(), mesh.numVerts, mesh.numFaces);

        // faces
        model.faces.append(subMesh.faces_);
        // add verts
        model.verts.append(subMesh.verts_);
        // add the mesh
        model.meshes.append(mesh);
    }

    model.endModel();
    // not needed anymore.
    areaMeshes.clear();

    // copy bounds.
    boundingBox = model.meshHeader.boundingBox;
    boundingSphere = model.meshHeader.boundingSphere;
    return true;
}

void LvlArea::addWindingForSide(const XPlaneSet& planes, const LvlBrushSide& side, Winding* pWinding)
{
    const auto& plane = planes[side.planenum];

    AreaSubMesh* pSubMesh = meshForSide(side);

    const size_t startVert = pSubMesh->verts_.size();
    const size_t numPoints = pWinding->getNumPoints();
    const model::Index offset = safe_static_cast<model::Index>(startVert);

    for (size_t i = 2; i < numPoints; i++) {
        for (size_t j = 0; j < 3; j++) {
            level::Vertex vert;

            if (j == 0) {
                const Vec5f& vec = pWinding->at(0);
                vert.pos = vec.asVec3();
                vert.texcoord[0] = Vec2f(vec.s, vec.t);
            }
            else if (j == 1) {
                const Vec5f& vec = pWinding->at(i - 1);
                vert.pos = vec.asVec3();
                vert.texcoord[0] = Vec2f(vec.s, vec.t);
            }
            else {
                const Vec5f& vec = pWinding->at(i);
                vert.pos = vec.asVec3();
                vert.texcoord[0] = Vec2f(vec.s, vec.t);
            }

            // copy normal
            vert.normal = plane.getNormal();
            vert.color = Col_White;

            pSubMesh->addVert(vert);
        }

        model::Index localOffset = safe_static_cast<model::Index>((i - 2) * 3);

        pSubMesh->faces_.emplace_back(
            offset + localOffset + 0_ui16,
            offset + localOffset + 1_ui16,
            offset + localOffset + 2_ui16);
    }
}

AreaSubMesh* LvlArea::meshForSide(const LvlBrushSide& side, StringTableUnique& stringTable)
{
    auto it = areaMeshes.find(X_CONST_STRING(side.matInfo.name.c_str()));
    if (it != areaMeshes.end()) {
        return &it->second;
    }
    // add new.
    AreaSubMesh newMesh;

    // add mat name to string table.
    newMesh.matNameID_ = stringTable.addStringUnqiue(side.matInfo.name.c_str());
    newMesh.matName_ = side.matInfo.name;

    auto newIt = areaMeshes.insert(
        AreaMeshMap::value_type(core::string(side.matInfo.name.c_str()), newMesh));

    return &newIt.first->second;
}

AreaSubMesh* LvlArea::meshForMat(const core::string& matName, StringTableUnique& stringTable)
{
    auto it = areaMeshes.find(matName);
    if (it != areaMeshes.end()) {
        return &it->second;
    }
    // add new.
    AreaSubMesh newMesh;

    // add mat name to string table.
    newMesh.matNameID_ = stringTable.addStringUnqiue(matName);
    newMesh.matName_ = MaterialName(matName.c_str());

    auto newIt = areaMeshes.insert(
        AreaMeshMap::value_type(matName, newMesh));

    return &newIt.first->second;
}

AreaSubMesh* LvlArea::meshForSide(const LvlBrushSide& side)
{
    auto it = areaMeshes.find(X_CONST_STRING(side.matInfo.name.c_str()));
    if (it != areaMeshes.end()) {
        return &it->second;
    }

    AreaSubMesh newMesh;
    newMesh.matNameID_ = 0;
    newMesh.matName_ = side.matInfo.name;

    core::string matStr(side.matInfo.name.begin(), side.matInfo.name.end());

    auto newIt = areaMeshes.insert({matStr, newMesh});

    return &newIt.first->second;
}

X_NAMESPACE_END