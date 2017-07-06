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

		for (size_t i = 0; i < numTransForms; i++)
		{
			auto& trans = pTransforms[i];
			X_ASSERT_NOT_NULL(trans.userData);
			EntityId ent = static_cast<EntityId>(union_cast<uintptr_t>(trans.userData) && 0xFFFF);

			auto& entTrans = reg.get<TransForm>(ent);

			entTrans.trans = trans.actor2World;
		}

	}

} // namespace entity

X_NAMESPACE_END