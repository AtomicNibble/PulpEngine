#include "stdafx.h"
#include "PhysicsSystem.h"

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IWorld3D.h>
#include <IModelManager.h>
#include <IEffect.h>

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

    void PhysicsSystem::update(core::FrameData& frame, EnitiyRegister& reg,
        physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld)
    {
        X_UNUSED(frame, reg, pPhysScene);

        core::span<const physics::ActiveTransform> transforms;
        core::span<const physics::TriggerPair> triggers;

        {
            physics::ScopedLock lock(pPhysScene, physics::LockAccess::Read);
            
            transforms = pPhysScene->getActiveTransforms();
            triggers = pPhysScene->getTriggerPairs();
        }

        for (int32_t i = 0; i < transforms.size(); i++) {
            auto& trans = transforms[i];

            if (!trans.userData) {
                continue;
            }

            EntityId ent = static_cast<EntityId>(union_cast<uintptr_t>(trans.userData) & 0xFFFF);

            auto& entTrans = reg.get<TransForm>(ent);

            entTrans.pos = trans.actor2World.pos;
            entTrans.quat = trans.actor2World.quat;

            if (reg.has<MeshRenderer>(ent)) {
                auto& rendEnt = reg.get<MeshRenderer>(ent);

                p3DWorld->updateRenderEnt(rendEnt.pRenderEnt, entTrans);
            }
            if (reg.has<Emitter>(ent)) {
                auto& emt = reg.get<Emitter>(ent);

                emt.pEmitter->setTrans(entTrans, emt.offset);
            }
        }

        for (int32_t i = 0; i < triggers.size(); i++) {
            auto& trigPair = triggers[i];
            X_UNUSED(trigPair);
        }
    }

    bool PhysicsSystem::createColliders(EnitiyRegister& reg, physics::IPhysics* pPhysics, physics::IScene* pPhysScene)
    {
        auto* pModelManager = gEnv->p3DEngine->getModelManager();

        core::ArrayGrowMultiply<physics::ActorHandle> actors(g_gameArena);
        actors.reserve(128);

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

                // TEMP: make all ents kinematic.
                col.actor = pPhysics->createActor(trans, true);

                mesh.pModel->addPhysToActor(col.actor);

                pPhysics->updateMassAndInertia(col.actor, 1.f);

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

                col.actor = pPhysics->createActor(trans, false, (void*)entity);

                if (!mesh.pModel->hasPhys()) {
                    X_ERROR("Ent", "Can't add mesh collider to ent with model \"%s\" it has no physics data", mesh.pModel->getName().c_str());
                    //	reg.remove<DynamicObject>(entity);
                    //	continue;

                    auto& bounds = mesh.pModel->bounds();
                    pPhysics->addBox(col.actor, bounds, bounds.center());
                }
                else {
                    mesh.pModel->addPhysToActor(col.actor);
                }

                pPhysics->updateMassAndInertia(col.actor, 5.f);

                actors.push_back(col.actor);
            }
        }

        if (actors.isNotEmpty()) {
            physics::ScopedLock lock(pPhysScene, physics::LockAccess::Write);

            pPhysScene->addActorsToScene(actors.data(), actors.size());
        }


        return true;
    }

} // namespace entity

X_NAMESPACE_END