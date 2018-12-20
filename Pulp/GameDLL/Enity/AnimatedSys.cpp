#include "stdafx.h"
#include "AnimatedSys.h"

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IWorld3D.h>
#include <IAnimManager.h>
#include <IEffect.h>

#include <Time\TimeLiterals.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{
    AnimatedSystem::AnimatedSystem()
    {
    }

    bool AnimatedSystem::init(void)
    {
        return true;
    }

    void AnimatedSystem::update(core::FrameTimeData& time, EnitiyRegister& reg,
        engine::IWorld3D* p3DWorld, physics::IScene* pPhysScene)
    {
        X_UNUSED(time, reg, p3DWorld);

#if 0
		static bool once = true;

		if(once)
		{
			once = false;

			auto* panimMan = gEnv->p3DEngine->getAnimManager();
			// auto* pAnim = panimMan->loadAnim("test/crow_dance_01");
		//	auto* pAnim = panimMan->loadAnim("test/duck_dance_01");
			auto* pAnim = panimMan->loadAnim("test/weap/mg42/fire");
			panimMan->waitForLoad(pAnim);

			auto view = reg.view<Animator, Mesh, TransForm>();
			for (auto entity : view)
			{
				auto& an = reg.get<Animator>(entity);
				X_ASSERT_NOT_NULL(an.pAnimator);

				an.pAnimator->playAnim(pAnim, time.ellapsed[core::Timer::GAME], 25000_ms, 100_ms);
				
			}
		}
#endif

        Matrix33f rotation;
        rotation.rotate(Vec3f::xAxis(), ::toRadians(90.f));
        rotation.rotate(Vec3f::yAxis(), ::toRadians(180.f));
        rotation.rotate(Vec3f::zAxis(), ::toRadians(180.f));
        // rotation.rotate(Vec3f::yAxis(), ::toRadians(90.f));
        // rotation.rotate(Vec3f::yAxis(), ::toRadians(90.f));

        const auto ellapsedTime = time.ellapsed[core::Timer::GAME];
        const auto delta = time.deltas[core::Timer::GAME];
        const float deltaSec = delta.GetSeconds();

        auto updateVis = [&](EntityId entity, const Transformf& trans) {
            if (reg.has<MeshRenderer>(entity)) {
                auto& rendEnt = reg.get<MeshRenderer>(entity);
                p3DWorld->updateRenderEnt(rendEnt.pRenderEnt, trans);
            }
            if (reg.has<Emitter>(entity)) {
                auto& emt = reg.get<Emitter>(entity);

                emt.pEmitter->setTrans(trans, emt.offset);
            }
            if (reg.has<DynamicObject>(entity)) {
                auto& col = reg.get<DynamicObject>(entity);

                if (col.kinematic)
                {
                    physics::ScopedLock lock(pPhysScene, physics::LockAccess::Write);

                    pPhysScene->setKinematicTarget(col.actor, trans);
                }
            }
        };

        {
            auto view = reg.view<Attached, TransForm>();
            for (auto entity : view) {
                auto& trans = reg.get<TransForm>(entity);
                auto& att = reg.get<Attached>(entity);

                if (!reg.isValid(att.parentEnt)) {
                    continue;
                }

                auto parent = att.parentEnt;

                // get the parent position.
                const auto& parTrans = reg.get<TransForm>(parent);

                trans = parTrans;
                trans.pos += att.offset;

                if (att.parentBone != model::INVALID_BONE_HANDLE) {
                    if (!reg.has<Animator>(parent)) {
                        continue;
                    }

                    auto& an = reg.get<Animator>(parent);
                    Vec3f bonePos;
                    Matrix33f boneAxis;

                    if (an.pAnimator->getBoneTransform(att.parentBone, ellapsedTime, bonePos, boneAxis)) {
                        trans.pos = parTrans.pos + bonePos * parTrans.quat;
                        trans.quat = Quatf(boneAxis) * trans.quat;
                    }
                }

                updateVis(entity, trans);
            }
        }

        {
            auto view = reg.view<Rotator, TransForm>();
            for (auto entity : view) {
                auto& trans = reg.get<TransForm>(entity);
                auto& rot = reg.get<Rotator>(entity);

                trans.quat *= Quatf(rot.axis, ::toRadians(rot.speed));

                updateVis(entity, trans);
            }
        }

        {
            auto view = reg.view<Mover, TransForm>();
            for (auto entity : view) {
                auto& trans = reg.get<TransForm>(entity);
                auto& mov = reg.get<Mover>(entity);

                // i want a value between 0-2 then i can shift it into back and forth.
                float deltaFraction = deltaSec / (mov.time * 2);

                // add on the faction.
                mov.fract += deltaFraction;
                if (mov.fract >= 1.f) {
                    mov.fract = -1.f;
                }

                float fract = math<float>::abs(mov.fract);
                trans.pos = mov.start.lerp(fract, mov.end);

                updateVis(entity, trans);
            }
        }

        auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::CONSOLE);

        auto view = reg.view<Animator, Mesh, MeshRenderer, TransForm>();
        for (auto entity : view) {
            auto& trans = reg.get<TransForm>(entity);
            auto& an = reg.get<Animator>(entity);
            auto& rendEnt = reg.get<MeshRenderer>(entity);

            X_UNUSED(pPrim, trans);
            //	rotation = Matrix33f::identity();
            //	an.pAnimator->renderInfo(ellapsedTime, Vec3f(1300.f, 100.f, 0.f), rotation, pPrim);

            if (!an.pAnimator->isAnimating(ellapsedTime)) {
                continue;
            }

            // so i only want to pump notes once.
            // and i also want to pump for a range,

            auto prevTime = ellapsedTime - delta;

            anim::NoteTrackValueArr values;
            an.pAnimator->getNotes(prevTime, ellapsedTime, values);

            if (values.isNotEmpty()) {
                // Weeeeeeee!
                for (auto* pValue : values) {
                    X_LOG0("Anim", "Note: \"%s\" Ent: %" PRId32, pValue, entity);

                    core::StringRange<char> str(pValue, pValue + core::strUtil::strlen(pValue));

                    if (str.getLength() > 4 && core::strUtil::IsEqual(str.getStart(), str.getStart() + 4, "snd#")) {
                        // it's a snd event.
                        const char* pSndEvent = str.getStart() + 4;

                        gEnv->pSound->postEvent(pSndEvent, sound::GLOBAL_OBJECT_ID);
                    }
                }
            }

            if (an.pAnimator->createFrame(ellapsedTime)) {
                auto& bones = an.pAnimator->getBoneMatrices();

                p3DWorld->setBonesMatrix(rendEnt.pRenderEnt, bones.data(), bones.size());
            }
        }
    }

} // namespace entity

X_NAMESPACE_END