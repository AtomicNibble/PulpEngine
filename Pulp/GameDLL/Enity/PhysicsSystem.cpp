#include "stdafx.h"
#include "PhysicsSystem.h"

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IModelManager.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{
    PhysicsSystem::PhysicsSystem() :
        pPhysScene_(nullptr)
    {
    }

    PhysicsSystem::~PhysicsSystem()
    {
    }

    bool PhysicsSystem::init(ECS& reg, physics::IScene* pPhysScene)
    {
        reg.registerHandler<PhysicsSystem, MsgMove>(this);

        pPhysScene_ = pPhysScene;
        return true;
    }

    void PhysicsSystem::update(core::FrameData& frame, ECS& reg)
    {
        X_UNUSED(frame, reg);

        core::span<const physics::ActiveTransform> transforms;
        core::span<const physics::TriggerPair> triggers;

        {
            physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Read);
            
            transforms = pPhysScene_->getActiveTransforms();
            triggers = pPhysScene_->getTriggerPairs();
        }

        for (int32_t i = 0; i < transforms.size(); i++) {
            auto& trans = transforms[i];

            if (trans.userData.getType() != physics::UserType::EntId) {
                continue;
            }

            EntityId ent = static_cast<EntityId>(trans.userData.getVal());

            // Currently I use foot position for players so this data would be wrong.
            // if we post a MSgMove for players with this they will rotate / move and flicker.
            if (ent < net::MAX_PLAYERS) {
                continue;
            }

            auto& entTrans = reg.get<TransForm>(ent);
            entTrans.pos = trans.actor2World.pos;
            entTrans.quat = trans.actor2World.quat;

            reg.dispatch<MsgMove>(ent);
        }

        for (int32_t i = 0; i < triggers.size(); i++) {
            auto& trigPair = triggers[i];
            X_UNUSED(trigPair);
            X_ASSERT_NOT_IMPLEMENTED();
        }
    }

    bool PhysicsSystem::createColliders(ECS& reg, physics::IPhysics* pPhysics, physics::IScene* pPhysScene)
    {
        auto* pModelManager = gEnv->p3DEngine->getModelManager();

        core::ArrayGrowMultiply<physics::ActorHandle> actors(g_gameArena);
        
        {
            size_t num = 0;

            // we can just count how big the arrays are.
            num += reg.view<MeshCollider>().size();
            num += reg.view<DynamicObject>().size();

            actors.reserve(num);
        }

        {
            auto view = reg.view<TransForm, Mesh, MeshCollider>();
            for (auto entity : view) {
                auto& trans = reg.get<TransForm>(entity);
                auto& mesh = reg.get<Mesh>(entity);
                auto& col = reg.get<MeshCollider>(entity);

                mesh.pModel->waitForLoad(pModelManager);

                if (!mesh.pModel->hasPhys()) {
                    X_ERROR("Ent", "Can't add mesh collider to ent with model \"%s\" it has no physics data", mesh.pModel->getName().c_str());
                    reg.remove<MeshCollider>(entity);
                    continue;
                }
                col.actor = pPhysics->createStaticActor(trans, physics::UserData(physics::UserType::EntId, entity));

                mesh.pModel->addPhysToActor(col.actor);

                actors.push_back(col.actor);
            }
        }

        {
            auto view = reg.view<TransForm, Mesh, DynamicObject>();

            for (auto entity : view) {
                auto& trans = reg.get<TransForm>(entity);
                auto& mesh = reg.get<Mesh>(entity);
                auto& col = reg.get<DynamicObject>(entity);

                mesh.pModel->waitForLoad(pModelManager);

                col.actor = pPhysics->createActor(trans, col.kinematic, physics::UserData(physics::UserType::EntId, entity));

                if (!mesh.pModel->hasPhys()) {
                    X_WARNING("Ent", "No physics shapes for model \"%s\" using model bounds", mesh.pModel->getName().c_str());

                    auto& bounds = mesh.pModel->bounds();
                    pPhysics->addBox(col.actor, bounds, bounds.center());
                }
                else {
                    mesh.pModel->addPhysToActor(col.actor);
                }

                pPhysics->updateMassAndInertia(col.actor, 1.f);

                actors.push_back(col.actor);
            }
        }

        if (actors.isNotEmpty()) {
            physics::ScopedLock lock(pPhysScene, physics::LockAccess::Write);
            pPhysScene->addActorsToScene(actors.data(), actors.size());
        }

        return true;
    }

    void PhysicsSystem::onMsg(ECS& reg, const MsgMove& msg)
    {
        if (!reg.has<DynamicObject>(msg.id)) {
            return;
        }

        // Kinematic actors have there positions set.
        auto& col = reg.get<DynamicObject>(msg.id);
        if (!col.kinematic) {
            return;
        }

        auto& trans = reg.get<TransForm>(msg.id);

        physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Write);
        pPhysScene_->setKinematicTarget(col.actor, trans);
    }

} // namespace entity

X_NAMESPACE_END