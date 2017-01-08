#include "stdafx.h"
#include "FilterShader.h"

X_NAMESPACE_BEGIN(physics)


namespace filter
{
	namespace
	{


	} // namespace


	physx::PxFilterFlags FilterShader(
		physx::PxFilterObjectAttributes attributes0,
		physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags,
		const void* constantBlock,
		physx::PxU32 constantBlockSize)
	{
		X_UNUSED(constantBlock);
		X_UNUSED(constantBlockSize);

		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlags();
		}


		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

		return physx::PxFilterFlags();
	}



} // namespace filter



X_NAMESPACE_END