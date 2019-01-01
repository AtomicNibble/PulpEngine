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

    void AnimatedSystem::update(core::FrameTimeData& time, ECS& reg,
        engine::IWorld3D* p3DWorld)
    {
        X_UNUSED(time, reg, p3DWorld);

        Matrix33f rotation;
        rotation.rotate(Vec3f::xAxis(), ::toRadians(90.f));
        rotation.rotate(Vec3f::yAxis(), ::toRadians(180.f));
        rotation.rotate(Vec3f::zAxis(), ::toRadians(180.f));
        // rotation.rotate(Vec3f::yAxis(), ::toRadians(90.f));
        // rotation.rotate(Vec3f::yAxis(), ::toRadians(90.f));

        const auto ellapsedTime = time.ellapsed[core::Timer::GAME];
        const auto delta = time.deltas[core::Timer::GAME];
        const float deltaSec = delta.GetSeconds();

        {
            auto view = reg.view<Attached, TransForm>();
            for (auto entity : view) {
                auto newTrans = reg.get<TransForm>(entity); // copy
                auto& att = reg.get<Attached>(entity);

                if (!reg.isValid(att.parentEnt)) {
                    continue;
                }

                auto parent = att.parentEnt;

                // get the parent position.
                const auto& parTrans = reg.get<TransForm>(parent);

                newTrans = parTrans;
                newTrans.pos += att.offset;

                if (att.parentBone != model::INVALID_BONE_HANDLE) {
                    if (!reg.has<Animator>(parent)) {
                        continue;
                    }

                    auto& an = reg.get<Animator>(parent);
                    Vec3f bonePos;
                    Matrix33f boneAxis;

                    if (an.pAnimator->getBoneTransform(att.parentBone, ellapsedTime, bonePos, boneAxis)) {
                        newTrans.pos = parTrans.pos + bonePos * parTrans.quat;
                        newTrans.quat = Quatf(boneAxis) * newTrans.quat;
                    }
                }

                auto& trans = reg.get<TransForm>(entity);
                if(newTrans != trans)
                {
                    trans = newTrans;
                    reg.dispatch<MsgMove>(entity);
                }
            }
        }

        {
            auto view = reg.view<Rotator, TransForm>();
            for (auto entity : view) {
                auto& trans = reg.get<TransForm>(entity);
                auto& rot = reg.get<Rotator>(entity);

                trans.quat *= Quatf(rot.axis, ::toRadians(rot.speed));

                reg.dispatch<MsgMove>(entity);
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

                reg.dispatch<MsgMove>(entity);
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