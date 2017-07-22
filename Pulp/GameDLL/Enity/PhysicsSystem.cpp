#include "stdafx.h"
#include "PhysicsSystem.h"

#include <IFrameData.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{


	PhysicsSystem::PhysicsSystem()
	{

	}

	PhysicsSystem::~PhysicsSystem()
	{

	}

	bool PhysicsSystem::init(void)
	{
	

		return true;
	}

	void PhysicsSystem::update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene)
	{
		X_UNUSED(frame, reg, pPhysScene);

		size_t numTransForms;
		auto* pTransforms = pPhysScene->getActiveTransforms(numTransForms);
		size_t numTriggerPairs;
		auto* pTriggerPairs = pPhysScene->getTriggerPairs(numTriggerPairs);


		for (size_t i = 0; i < numTransForms; i++)
		{
			auto& trans = pTransforms[i];
			X_ASSERT_NOT_NULL(trans.userData);
			EntityId ent = static_cast<EntityId>(union_cast<uintptr_t>(trans.userData) && 0xFFFF);

			auto& entTrans = reg.get<TransForm>(ent);

			entTrans.pos = trans.actor2World.pos;
			entTrans.quat = trans.actor2World.quat;
		}

		for (size_t i = 0; i < numTriggerPairs; i++)
		{
			auto& trigPair = pTriggerPairs[i];
			X_UNUSED(trigPair);

		}

	}

} // namespace entity

X_NAMESPACE_END