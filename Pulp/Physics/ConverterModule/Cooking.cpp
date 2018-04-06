#include "stdafx.h"
#include "Cooking.h"

X_NAMESPACE_BEGIN(physics)

namespace
{
    X_INLINE physx::PxBoundedData PxBoundedDataFromBounded(const BoundedData& in)
    {
        physx::PxBoundedData bd;
        bd.data = in.pData;
        bd.stride = in.stride;
        bd.count = in.count;
        return bd;
    }

} // namespace

PhysCooking::PhysCooking(core::MemoryArenaBase* arena) :
    arena_(arena),
    physAllocator_(arena),
    pCooking_(nullptr)
{
}

PhysCooking::~PhysCooking()
{
    shutDown();
}

bool PhysCooking::init(const physx::PxTolerancesScale& scale, physx::PxFoundation& foundation, CookingMode::Enum mode)
{
    scale_ = scale;

    physx::PxCookingParams params(scale);
    setCookingParamsForMode(params, mode);

    pCooking_ = PxCreateCooking(PX_PHYSICS_VERSION, foundation, params);
    if (!pCooking_) {
        X_ERROR("Physics", "PxCreateCooking failed!");
        return false;
    }

    return true;
}

void PhysCooking::shutDown(void)
{
    core::SafeRelease(pCooking_);
}

bool PhysCooking::cookingSupported(void) const
{
    return true;
}

bool PhysCooking::setCookingMode(CookingMode::Enum mode)
{
    static_assert(CookingMode::ENUM_COUNT == 3, "Added additional cooking modes? this code needs updating.");

    if (!pCooking_) {
        X_ERROR("Phys", "Can;t set cooking mode before cooking is init");
        return false;
    }

    physx::PxCookingParams params(scale_);
    setCookingParamsForMode(params, mode);

    pCooking_->setParams(params);
    return true;
}

bool PhysCooking::cookTriangleMesh(const TriangleMeshDesc& desc, DataArr& dataOut, CookFlags flags)
{
    X_ASSERT_NOT_NULL(pCooking_);

    physx::PxTriangleMeshDesc meshDesc;
    meshDesc.points = PxBoundedDataFromBounded(desc.points);
    meshDesc.triangles = PxBoundedDataFromBounded(desc.triangles);

    if (flags.IsSet(CookFlag::INDICES_16BIT)) {
        meshDesc.flags.set(physx::PxMeshFlag::e16_BIT_INDICES);
    }

    if (!meshDesc.isValid()) {
        X_ERROR("Phys", "Failed to cook tri mesh, description is invalid");
        return false;
    }

    if (pCooking_->getParams().meshPreprocessParams & physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH) {
        if (!pCooking_->validateTriangleMesh(meshDesc)) {
            X_ERROR("Phys", "Failed to cook tri mesh, validation failed");
            return false;
        }
    }

    // i could props make my own outputstream wrapper that writes directly to the dataOut.
    // cba for now.
    physx::PxDefaultMemoryOutputStream writeBuffer;
    bool status = pCooking_->cookTriangleMesh(meshDesc, writeBuffer);
    if (!status) {
        X_ERROR("Phys", "Failed to cook tri mesh");
        return false;
    }

    dataOut.resize(writeBuffer.getSize());
    std::memcpy(dataOut.data(), writeBuffer.getData(), writeBuffer.getSize());
    return true;
}

bool PhysCooking::cookConvexMesh(const TriangleMeshDesc& triDesc, DataArr& dataOut, CookFlags flags)
{
    // we support cooking a convex mesh from a triangle representation.
    physx::PxSimpleTriangleMesh triMesh;
    triMesh.points = PxBoundedDataFromBounded(triDesc.points);
    triMesh.triangles = PxBoundedDataFromBounded(triDesc.triangles);

    if (flags.IsSet(CookFlag::INDICES_16BIT)) {
        triMesh.flags.set(physx::PxMeshFlag::e16_BIT_INDICES);
    }

    if (!triMesh.isValid()) {
        X_ERROR("Phys", "Failed to cook convex mesh, tri mesh is invalid");
        return false;
    }

    physx::PxU32 numVerts = 0;
    physx::PxVec3* pVertices = nullptr;
    physx::PxU32 numIndices = 0;
    physx::PxU32* pIndices = nullptr;
    physx::PxU32 numPolygons = 0;
    physx::PxHullPolygon* pPolygons = nullptr;

    if (!pCooking_->computeHullPolygons(
            triMesh,
            physAllocator_,
            numVerts, pVertices,
            numIndices, pIndices,
            numPolygons, pPolygons)) {
        X_ERROR("Phys", "Failed to compute convex hull for tri mesh");
        return false;
    }

    // now we bake convex.
    physx::PxConvexMeshDesc meshDesc;
    meshDesc.points.data = pVertices;
    meshDesc.points.count = numVerts;
    meshDesc.points.stride = sizeof(physx::PxVec3);
    // even if we where given 16bit indices, computeHullPolygons will give us 32bit ones.
    meshDesc.indices.data = pIndices;
    meshDesc.indices.count = numIndices;
    meshDesc.indices.stride = sizeof(physx::PxU32);
    meshDesc.polygons.data = pPolygons;
    meshDesc.polygons.count = numPolygons;
    meshDesc.polygons.stride = sizeof(physx::PxHullPolygon);

    if (!meshDesc.isValid()) {
        X_ERROR("Phys", "Failed to cook convex mesh, description is invalid");
        return false;
    }

    physx::PxDefaultMemoryOutputStream writeBuffer;
    bool status = pCooking_->cookConvexMesh(meshDesc, writeBuffer);
    if (!status) {
        physAllocator_.deallocate(pVertices);
        physAllocator_.deallocate(pIndices);
        physAllocator_.deallocate(pPolygons);
        X_ERROR("Phys", "Failed to cook convex mesh");
        return false;
    }

    physAllocator_.deallocate(pVertices);
    physAllocator_.deallocate(pIndices);
    physAllocator_.deallocate(pPolygons);

    dataOut.resize(writeBuffer.getSize());
    std::memcpy(dataOut.data(), writeBuffer.getData(), writeBuffer.getSize());
    return true;
}

bool PhysCooking::cookConvexMesh(const ConvexMeshDesc& desc, DataArr& dataOut, CookFlags flags)
{
    physx::PxConvexMeshDesc meshDesc;
    meshDesc.points = PxBoundedDataFromBounded(desc.points);
    meshDesc.polygons = PxBoundedDataFromBounded(desc.polygons);
    meshDesc.indices = PxBoundedDataFromBounded(desc.indices);

    if (flags.IsSet(CookFlag::INDICES_16BIT)) {
        meshDesc.flags.set(physx::PxConvexFlag::e16_BIT_INDICES);
    }
    if (flags.IsSet(CookFlag::COMPUTE_CONVEX)) {
        meshDesc.flags.set(physx::PxConvexFlag::eCOMPUTE_CONVEX);
    }

    if (!meshDesc.isValid()) {
        X_ERROR("Phys", "Failed to cook convex mesh, description is invalid");
        return false;
    }

    physx::PxDefaultMemoryOutputStream writeBuffer;
    bool status = pCooking_->cookConvexMesh(meshDesc, writeBuffer);
    if (!status) {
        X_ERROR("Phys", "Failed to cook convex mesh");
        return false;
    }

    dataOut.resize(writeBuffer.getSize());
    std::memcpy(dataOut.data(), writeBuffer.getData(), writeBuffer.getSize());
    return true;
}

bool PhysCooking::cookHeightField(const HeightFieldDesc& desc, DataArr& dataOut)
{
    physx::PxHeightFieldDesc hfDesc;
    hfDesc.format = physx::PxHeightFieldFormat::eS16_TM;
    hfDesc.nbColumns = desc.numCols;
    hfDesc.nbRows = desc.numRows;
    hfDesc.samples.data = desc.samples.pData;
    hfDesc.samples.stride = sizeof(physx::PxHeightFieldSample);

    if (!hfDesc.isValid()) {
        X_ERROR("Phys", "Failed to cook height field, description is invalid");
        return false;
    }

    const uint32_t numSamples = desc.numCols * desc.numRows;

    core::Array<physx::PxHeightFieldSample> samples(arena_);
    samples.resize(numSamples);

    const HeightFieldSample* pSamplers = reinterpret_cast<const HeightFieldSample*>(desc.samples.pData);

    for (uint32_t i = 0; i < numSamples; i++) {
        auto& destSampler = samples[i];

        destSampler.height = pSamplers[i].height;
        destSampler.materialIndex0 = pSamplers[i].matIdx0;
        destSampler.materialIndex1 = pSamplers[i].matIdx1;
    }

    physx::PxDefaultMemoryOutputStream writeBuffer;
    bool status = pCooking_->cookHeightField(hfDesc, writeBuffer);
    if (!status) {
        X_ERROR("Phys", "Failed to cook height field");
        return false;
    }

    dataOut.resize(writeBuffer.getSize());
    std::memcpy(dataOut.data(), writeBuffer.getData(), writeBuffer.getSize());
    return true;
}

void PhysCooking::setCookingParamsForMode(physx::PxCookingParams& params, CookingMode::Enum mode)
{
    params.targetPlatform = physx::PxPlatform::ePC;
    params.meshWeldTolerance = 0.001f;

    switch (mode) {
        case CookingMode::Fast:
            params.meshCookingHint = physx::PxMeshCookingHint::eSIM_PERFORMANCE;
            params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
                physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH | physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
            break;

        case CookingMode::Slow:
            params.meshCookingHint = physx::PxMeshCookingHint::eSIM_PERFORMANCE;
            params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
                physx::PxMeshPreprocessingFlag::eWELD_VERTICES | physx::PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES | physx::PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES);
            break;

        case CookingMode::VerySlow:
            params.meshCookingHint = physx::PxMeshCookingHint::eSIM_PERFORMANCE;
            params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
                physx::PxMeshPreprocessingFlag::eWELD_VERTICES | physx::PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES | physx::PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES);
            break;

        default:
            X_ASSERT_UNREACHABLE();
            break;
    }
}

X_NAMESPACE_END