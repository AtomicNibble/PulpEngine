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
	pCooking_(nullptr)
{

}

PhysCooking::~PhysCooking()
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

bool PhysCooking::cookTriangleMesh(const TriangleMeshDesc& desc, DataArr& dataOut)
{
	X_ASSERT_NOT_NULL(pCooking_);

	physx::PxTriangleMeshDesc meshDesc;
	meshDesc.points = PxBoundedDataFromBounded(desc.points);
	meshDesc.triangles = PxBoundedDataFromBounded(desc.triangles);

	if (!meshDesc.isValid()) {
		X_ERROR("Phys", "Failed to cook tri mesh, description is invalid");
		return false;
	}

	if (pCooking_->getParams().meshPreprocessParams & physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH)
	{
		if (!pCooking_->validateTriangleMesh(meshDesc))
		{
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


bool PhysCooking::cookConvexMesh(const ConvexMeshDesc& desc, DataArr& dataOut)
{
	physx::PxConvexMeshDesc meshDesc;
	meshDesc.points = PxBoundedDataFromBounded(desc.points);
	meshDesc.indices = PxBoundedDataFromBounded(desc.indices);


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

	for (uint32_t i = 0; i < numSamples; i++)
	{
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

	switch (mode)
	{
		case CookingMode::Fast:
			params.meshCookingHint = physx::PxMeshCookingHint::eSIM_PERFORMANCE;
			params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
				physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH |
				physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
			break;

		case CookingMode::Slow:
			params.meshCookingHint = physx::PxMeshCookingHint::eSIM_PERFORMANCE;
			params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
				physx::PxMeshPreprocessingFlag::eWELD_VERTICES |
				physx::PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES |
				physx::PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES);
			break;

		case CookingMode::VerySlow:
			params.meshCookingHint = physx::PxMeshCookingHint::eSIM_PERFORMANCE;
			params.meshPreprocessParams = physx::PxMeshPreprocessingFlags(
				physx::PxMeshPreprocessingFlag::eWELD_VERTICES |
				physx::PxMeshPreprocessingFlag::eREMOVE_UNREFERENCED_VERTICES |
				physx::PxMeshPreprocessingFlag::eREMOVE_DUPLICATED_TRIANGLES);
			break;

		default:
			X_ASSERT_UNREACHABLE();
			break;
	}
}

X_NAMESPACE_END