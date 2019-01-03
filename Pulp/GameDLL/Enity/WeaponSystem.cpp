#include "stdafx.h"
#include "WeaponSystem.h"
#include "Weapon\WeaponDef.h"

#include "Vars/GameVars.h"

#include <IAnimManager.h>
#include <IFrameData.h>
#include <I3DEngine.h>
#include <Time\TimeLiterals.h>
#include <Hashing\Fnva1Hash.h>

#include <IPrimativeContext.h>
#include <IFont.h>
#include <IEffect.h>
#include <IPhysics.h>

using namespace core::Hash::Literals;
using namespace sound::Literals;

X_NAMESPACE_BEGIN(game)

namespace entity
{
    WeaponSystem::WeaponSystem(GameVars& vars) :
        vars_(vars),
        pReg_(nullptr),
        pPhysScene_(nullptr),
        pAnimManager_(nullptr)
    {
        
    }

    bool WeaponSystem::init(ECS& reg, physics::IScene* pPhysScene, anim::IAnimManager* pAnimManager)
    {
        pReg_ = &reg;
        pPhysScene_ = pPhysScene;
        pAnimManager_ = pAnimManager;
        return true;
    }

    void WeaponSystem::update(core::FrameData& frame, ECS& reg)
    {
        auto curTime = frame.timeInfo.ellapsed[core::Timer::GAME];

        auto view = reg.view<Weapon>();
        for (auto entity : view) {
            auto& wpnTrans = reg.get<TransForm>(entity);
            auto& wpn = reg.get<Weapon>(entity);
            auto& animator = reg.get<Animator>(entity);

            auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::GUI);

            core::StackString256 ammoText;

            auto ammoType = wpn.pWeaponDef->getAmmoTypeId();

            {
                auto& inv = pReg_->get<Inventory>(wpn.ownerEnt);

                ammoText.appendFmt("Name: %s\nAmmoInClip: %i\nAmmoStore: %i", wpn.pWeaponDef->getStrSlot(weapon::StringSlot::DisplayName),
                    wpn.ammoInClip, inv.numAmmo(ammoType));
            }

            font::TextDrawContext con;
            con.col = Col_Mintcream;
            con.size = Vec2f(24.f, 24.f);
            con.effectId = 0;
            con.pFont = gEnv->pFontSys->getDefault();

            // update the emitter positions.
            {
                // gimmy the bone positions!
                auto flashBone = animator.pAnimator->getBoneHandle("tag_flash");
                if (flashBone != model::INVALID_BONE_HANDLE) {

                    Vec3f bonePos;
                    Matrix33f boneAxis;

                    if (animator.pAnimator->getBoneTransform(flashBone, curTime, bonePos, boneAxis)) {
                        //		boneAxis.rotate(Vec3f::xAxis(), ::toRadians(180.f));

                        Transformf trans;
                        trans.pos = wpnTrans.pos + bonePos * wpnTrans.quat;
                        trans.quat = wpnTrans.quat * Quatf(boneAxis);

#if 0
						core::StackString256 test;
						test.setFmt("(%f, %f, %f) %f", trans.quat.v.x, trans.quat.v.y, trans.quat.v.z, trans.quat.w);
						pPrim->drawText(Vec3f(5.f, 900.f, 1.f), con, test.begin(), test.end());
#endif

                        wpn.pFlashEmt->setTrans(trans);
                    }
                }
            }

            // pPrim->drawQuad(400.f, 400.f, 256, 128, wpn.pWeaponDef->getIcon(weapon::IconSlot::AmmoCounter), Color(0.2f, 0.2f, 0.2f));

            auto res = frame.view.displayRes;

            pPrim->drawText(Vec3f(5.f, res.y - 80.f, 1.f), con, ammoText.begin(), ammoText.end());

            /*
				so everything kinda works nice anim wise if we play the same animation on both arms and weapon.
				but in this contenxt we only have the weapon.

				should the weapon look for arms or should the player copy the anims onto arms.
				i think for now weapon will look for player, and get it's arms.

				will a weapon work when not attached to a player?
				like it might belong to a bot or .. ?

				depends if i make turrets a seperate thing.
				what if you take a wepaon and throw it on the wall like a laptop gun
				would that just become a turret? or stay a gun.
				dunno.

				but a weapon ent don't know it's owner currently.
			*/

            switch (wpn.state) {
                case weapon::State::Lowering:
                    if (curTime >= wpn.stateEnd) {
                        wpn.state = weapon::State::Holstered;
                    }
                    break;
                case weapon::State::Holstered:
                    if (!wpn.holster) { // curTime >= wpn.stateEnd) {
                        beginRaise(curTime, wpn, animator);
                    }
                    break;
                case weapon::State::Raising:
                    if (curTime >= wpn.stateEnd) {
                        beginIdle(curTime, wpn, animator);
                    }
                    break;
                case weapon::State::Fire:
                    if (curTime >= wpn.stateEnd) {
                        if (wpn.attack) {
                            beginAttack(curTime, wpn, animator, wpnTrans, frame);
                        }
                        else {
                            beginIdle(curTime, wpn, animator);
                        }
                    }
                    break;
                case weapon::State::OutOfAmmo:
                    if (curTime >= wpn.stateEnd) {
                        beginIdle(curTime, wpn, animator);
                    }
                    break;
                case weapon::State::Reloading:
                    if (curTime >= wpn.stateEnd) {
                        // add the ammo post reload.
                        if (wpn.ownerEnt != INVALID_ENT_ID && pReg_->has<Inventory>(wpn.ownerEnt)) {
                            auto& inv = pReg_->get<Inventory>(wpn.ownerEnt);

                            int32_t avaliableAmmo = inv.numAmmo(ammoType); 
                            int32_t clipSize = wpn.pWeaponDef->getAmmoSlot(weapon::AmmoSlot::ClipSize);
                            int32_t clipSpace = clipSize - wpn.ammoInClip;
                            int32_t ammoToAdd = core::Min(clipSpace, core::Min(clipSize, avaliableAmmo));

                            if (ammoToAdd != 0) {
                                wpn.ammoInClip += ammoToAdd;
                                inv.useAmmo(ammoType, ammoToAdd);
                            }
                        }

                        wpn.reload = false;
                        beginIdle(curTime, wpn, animator);
                    }
                    break;
                case weapon::State::Idle: {
                    // if we are idle we can trainsition to:
                    // fire, drop, reload
                    // the weapon can be put into fire mode via various things.
                    // like the player could start the attack.
                    // and the animation / sounds are handled here.
                    if (wpn.stateEnd + 3000_ms < curTime) {
                        //	beginLower(curTime, wpn, animator);
                        //	continue;
                    }


                    if (wpn.holster) {
                        wpn.holster = false;
                        beginLower(curTime, wpn, animator);
                    }
                    else if (wpn.reload) {
                        wpn.reload = false;
                        beginReload(curTime, wpn, animator);
                    }
                    else if (wpn.attack) {
                        beginAttack(curTime, wpn, animator, wpnTrans, frame);
                    }

                } break;
            }
        }
    }

    void WeaponSystem::beginRaise(core::TimeVal curTime, Weapon& wpn, Animator& animator)
    {
        X_ASSERT(wpn.state == weapon::State::Holstered, "Invalid state")(wpn.state);

        auto raiseEvt = wpn.pWeaponDef->getSoundSlotHash(weapon::SoundSlot::Raise);
        if (raiseEvt) {
            gEnv->pSound->postEvent(raiseEvt, sound::GLOBAL_OBJECT_ID);
        }

        auto raiseTime = wpn.pWeaponDef->stateTimer(weapon::StateTimer::Raise);

        weapon::AnimSlot::Enum anim = weapon::AnimSlot::Raise;
        if (!wpn.stateFlags.IsSet(weapon::StateFlag::HasRaised)) {
            wpn.stateFlags.Set(weapon::StateFlag::HasRaised);

            // use first raise anim if we have.
            // also only use the first raise time if anim present?
            if (wpn.pWeaponDef->hasAnimSlot(weapon::AnimSlot::FirstRaise)) {
                anim = weapon::AnimSlot::FirstRaise;
                raiseTime = wpn.pWeaponDef->stateTimer(weapon::StateTimer::FirstRaise);
            }
        }

        trainsitionToState(wpn, animator, anim, weapon::State::Raising, curTime, raiseTime);
    }

    void WeaponSystem::beginLower(core::TimeVal curTime, Weapon& wpn, Animator& animator)
    {
        auto lowerEvt = wpn.pWeaponDef->getSoundSlotHash(weapon::SoundSlot::Lower);
        if (lowerEvt) {
            gEnv->pSound->postEvent(lowerEvt, sound::GLOBAL_OBJECT_ID);
        }

        auto raiseTime = wpn.pWeaponDef->stateTimer(weapon::StateTimer::Lower);

        trainsitionToState(wpn, animator, weapon::AnimSlot::Lower, weapon::State::Lowering, curTime, raiseTime);
    }

    void WeaponSystem::beginIdle(core::TimeVal curTime, Weapon& wpn, Animator& animator)
    {
        trainsitionToState(wpn, animator, weapon::AnimSlot::Idle, weapon::State::Idle, curTime, 0_ms);
    }

    void WeaponSystem::beginAttack(core::TimeVal curTime, Weapon& wpn, Animator& animator, TransForm& wpnTrans, core::FrameData& frame)
    {
        X_UNUSED(frame);

        // need to check ammo.
        if (wpn.ammoInClip == 0) {
            X_LOG0("Weapon", "Clip empty");

            if (!beginReload(curTime, wpn, animator)) {
                auto sndEvt = wpn.pWeaponDef->getSoundSlotHash(weapon::SoundSlot::EmptyFire);
                if (sndEvt) {
                    gEnv->pSound->postEvent(sndEvt, sound::GLOBAL_OBJECT_ID);
                }

                wpn.state = weapon::State::OutOfAmmo;
                wpn.stateEnd = curTime + 1000_ms;
            }
            return;
        }

        --wpn.ammoInClip;

        auto anim = weapon::AnimSlot::Fire;
        auto snd = weapon::SoundSlot::Fire;

        if (wpn.ammoInClip == 0) {
            if (wpn.pWeaponDef->hasAnimSlot(weapon::AnimSlot::LastShot)) {
                anim = weapon::AnimSlot::LastShot;
            }
            if (wpn.pWeaponDef->hasSoundSlot(weapon::SoundSlot::LastShot)) {
                snd = weapon::SoundSlot::LastShot;
            }
        }

        auto fireTime = wpn.pWeaponDef->stateTimer(weapon::StateTimer::Fire);

        auto sndEvt = wpn.pWeaponDef->getSoundSlotHash(snd);
        if (sndEvt) {
            gEnv->pSound->postEvent(sndEvt, sound::GLOBAL_OBJECT_ID);
        }

        auto* pViewEffect = wpn.pWeaponDef->getEffect(weapon::EffectSlot::FlashView);
        if (pViewEffect) {
            wpn.pFlashEmt->play(pViewEffect, false, false);
        }

        // lets do a ray cast here.
        // no idea where is best to do this currently.
        {
            float maxRange = static_cast<float>(wpn.pWeaponDef->maxDmgRange());

            // where do i want to fire from?
            // the end of the gun?
            // or just my view.
            Vec3f origin = wpnTrans.pos;
            Vec3f uintDir = (wpnTrans.quat * Vec3f::xAxis());
            float distance = maxRange;

            // TODO: use a bone for ray origin?

            physics::RaycastBuffer hit;
            physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Write);

            // TODO: support dynamic without hitting players own collion
            if (pPhysScene_->raycast(
                    origin + (uintDir * 30.f),
                    uintDir,
                    distance,
                    hit,
                    physics::DEFAULT_HIT_FLAGS,
                    physics::QueryFlag::DYNAMIC | physics::QueryFlag::STATIC)) 
            {
                auto b = hit.block;

                X_LOG0("Weapon", "Hit %f (%f,%f, %f)", b.distance, b.position.x, b.position.y, b.position.z);

                if (vars_.drawBulletRay()) {
                    auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PERSISTENT);
                    pPrim->drawLine(origin, b.position, Col_Red);
                    pPrim->drawAxis(b.position, Vec3f(15.f));
                }

                // so i would like to apply forces to dynamic objects.
                // would nice to be able to check if actor is dynamic. :/
                // actually i need to know if dynamic and not kinemetic, other wise physicx bitch.
                // so basically need acess to flags.

                auto meta = gEnv->pPhysics->getActorMeta(b.actor);
                if (meta.type == physics::ActorType::Dynamic)
                {
                    if (!meta.flags.IsSet(physics::ActorFlags::Kinematic))
                    {
                        auto dir = b.position - origin;
                        dir.normalize();
                        dir *= 9999000.f; // TODO: de fuck

                        pPhysScene_->addForce(b.actor, dir);
                    }

                    if (meta.pUserData)
                    {
                        // Assume valid?
                        EntityId id = static_cast<EntityId>(reinterpret_cast<uintptr_t>(meta.pUserData) & 0xFFFF);
                        
                        // can only do dmg to stuff that has HP.
                        // should we do this check for clients tho?
                        if (pReg_->has<Health>(id)) {
                            doDamage(id, wpn, b.distance);
                        }
                    }
                }
            }
        }

        trainsitionToState(wpn, animator, anim, weapon::State::Fire, curTime, fireTime);
    }

    void WeaponSystem::doDamage(EntityId ent, Weapon& wpn, float distance)
    {
        auto* pWeaponDef = wpn.pWeaponDef;

        float minRange = static_cast<float>(pWeaponDef->minDmgRange());
        float maxRange = static_cast<float>(pWeaponDef->maxDmgRange());
        
        if (distance > maxRange) {
            X_WARNING("Weapon", "Skipping damage distance is greater than weapons max range.");
            return;
        }

        int32_t dmg = 0;

        // TODO: should dmg drop linearly between min and max?
        if (distance <= minRange) {
            dmg = pWeaponDef->maxDmg();
        }
        else {
            dmg = pWeaponDef->minDmg();
        }

        // Only the server and single player should actually do damage.
        bool isHost = true;

        if (isHost)
        {
            // do the dmg.
            serverDoDamage(ent, wpn, dmg);
        }
        else
        {
            // tell the server about it.
            X_LOG0("Weapon", "Client hit Dmg: %" PRIi32, dmg);
        }
    }

    void WeaponSystem::serverDoDamage(EntityId ent, Weapon& wpn, int32_t dmg)
    {
        if (dmg <= 0) {
            return;
        }

        // TODO: support a weapon without a owner aka a turret?
        auto attacker = wpn.ownerEnt;

        pReg_->dispatch<MsgDamage>(ent, attacker, dmg);
    }

    bool WeaponSystem::beginReload(core::TimeVal curTime, Weapon& wpn, Animator& animator)
    {
        // can we reload?
        // check if we have ammo, but don't add it yet.
        {
            if (wpn.ownerEnt == INVALID_ENT_ID || !pReg_->has<Inventory>(wpn.ownerEnt)) {
                return false;
            }

            auto ammoType = wpn.pWeaponDef->getAmmoTypeId();

            auto& inv = pReg_->get<Inventory>(wpn.ownerEnt);
            int32_t avaliableAmmo = inv.numAmmo(ammoType); 

            if (avaliableAmmo == 0) {
                return false;
            }

            int32_t clipSize = wpn.pWeaponDef->getAmmoSlot(weapon::AmmoSlot::ClipSize);
            int32_t clipSpace = clipSize - wpn.ammoInClip;

            // already full.
            if (clipSpace == 0) {
                return false;
            }
        }

        auto reloadTime = wpn.pWeaponDef->stateTimer(weapon::StateTimer::Reload);

        trainsitionToState(wpn, animator, weapon::AnimSlot::Reload, weapon::State::Reloading, curTime, reloadTime);
        return true;
    }

    void WeaponSystem::trainsitionToState(Weapon& wpn, Animator& animator, weapon::AnimSlot::Enum animSlot, weapon::State::Enum newState,
        core::TimeVal curTime, core::TimeVal transTime)
    {
        //	X_LOG0("Weapon", "State change %s -> %s", weapon::State::ToString(wpn.state), weapon::State::ToString(newState));

        wpn.state = newState;
        wpn.stateEnd = curTime + transTime;

        if (!wpn.pWeaponDef->hasAnimSlot(animSlot)) {
            X_ERROR("Weapon", "no animation set for: %s", weapon::AnimSlot::ToString(animSlot));
            return;
        }

        auto* pAnim = wpn.pWeaponDef->getAnim(animSlot);

        pAnim->waitForLoad(pAnimManager_);

        if (transTime == 0_ms) {
            animator.pAnimator->playAnim(pAnim, curTime, 0_ms);
        }
        else {
            animator.pAnimator->playAnim(pAnim, curTime, 0_ms, transTime);
        }

        // some shitty logic to check if player has arms, and apply same animation.
        if (wpn.ownerEnt == INVALID_ENT_ID || !pReg_->has<Player>(wpn.ownerEnt)) {
            return;
        }

        const auto& player = pReg_->get<Player>(wpn.ownerEnt);
        if (player.armsEnt == INVALID_ENT_ID || !pReg_->has<Animator>(player.armsEnt)) {
            return;
        }

        const auto& armsAnimator = pReg_->get<Animator>(player.armsEnt);

        if (transTime == 0_ms) {
            armsAnimator.pAnimator->playAnim(pAnim, curTime, 0_ms);
        }
        else {
            armsAnimator.pAnimator->playAnim(pAnim, curTime, 0_ms, transTime);
        }
    }

} // namespace entity

X_NAMESPACE_END
