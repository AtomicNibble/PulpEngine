#include "stdafx.h"
#include "PhysicsSystem.h"

#include <IFrameData.h>
#include <I3DEngine.h>

#include <IModelManager.h>

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

	bool PhysicsSystem::createColliders(EnitiyRegister& reg, physics::IPhysics* pPhysics, physics::IScene* pPhysScene)
	{
		auto* pModelManager = gEnv->p3DEngine->getModelManager();

		auto view = reg.view<TransForm, Mesh, MeshCollider>();
		for (auto entity : view)
		{
			auto& trans = reg.get<TransForm>(entity);
			auto& mesh = reg.get<Mesh>(entity);
			auto& col = reg.get<MeshCollider>(entity);

			mesh.pModel->waitForLoad(pModelManager);

			if (!mesh.pModel->hasPhys())
			{
				X_ERROR("Ent", "Can't add mesh collider to ent with model \"%s\" it has no physics data", mesh.pModel->getName().c_str());
				reg.remove<MeshCollider>(entity);
				continue;
			}

			// TEMP: make all ents kinematic.
		//	col.actor = pPhysics->createStaticActor(trans);
			col.actor = pPhysics->createActor(trans, 1.f, true);

			mesh.pModel->addPhysToActor(col.actor);

			pPhysScene->addActorToScene(col.actor);
		}

		return true;
	}

} // namespace entity

X_NAMESPACE_END