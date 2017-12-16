#include "stdafx.h"
#include "WeaponSystem.h"

#include "Weapon\WeaponDef.h"
#include <IAnimManager.h>
#include <IFrameData.h>
#include <I3DEngine.h>
#include <Time\TimeLiterals.h>
#include <Hashing\Fnva1Hash.h>

#include <IPrimativeContext.h>
#include <IFont.h>

using namespace core::Hash::Fnv1Literals;
using namespace sound::literals;


X_NAMESPACE_BEGIN(game)

namespace entity
{

	WeaponSystem::WeaponSystem() :
		pAnimManager_(nullptr)
	{
		pReg_ = nullptr;
	}


	bool WeaponSystem::init(void)
	{
		pAnimManager_ = gEnv->p3DEngine->getAnimManager();

		return true;
	}

	void WeaponSystem::update(core::FrameTimeData& time, EnitiyRegister& reg)
	{
		X_UNUSED(time, reg);
		pReg_ = &reg;

		auto curTime = time.ellapsed[core::Timer::GAME];


		auto view = reg.view<Weapon>();
		for (auto entity : view)
		{
			auto& wpn = reg.get<Weapon>(entity);
			auto& animator = reg.get<Animator>(entity);

			X_UNUSED(wpn);

			auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::GUI);


			core::StackString256 ammoText;
	
			{
				auto& inv = pReg_->get<Inventory>(wpn.ownerEnt);
				ammoText.appendFmt("AmmoInClip: %i\nAmmoStore: %i", wpn.ammoInClip, inv.ammo);
			}
			font::TextDrawContext con;
			con.col = Col_Mintcream;
			con.size = Vec2f(24.f, 24.f);
			con.effectId = 0;
			con.pFont = gEnv->pFontSys->GetDefault();
			

			pPrim->drawText(Vec3f(10.f, 500.f, 1.f), con, ammoText.begin(), ammoText.end());

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

			switch (wpn.state)
			{
				case weapon::State::Lowering:
					if (curTime >= wpn.stateEnd)
					{
						wpn.state = weapon::State::Holstered;
					}
					break;
				case weapon::State::Holstered:
					if (curTime >= wpn.stateEnd)
					{
						beginRaise(curTime, wpn, animator);
					}
					break;
				case weapon::State::Raising:
					if (curTime >= wpn.stateEnd)
					{
						beginIdle(curTime, wpn, animator);
					}
					break;
				case weapon::State::Fire:
					if (curTime >= wpn.stateEnd)
					{
						if (wpn.attack)
						{
							beginAttack(curTime, wpn, animator);
						}
						else
						{
							beginIdle(curTime, wpn, animator);
						}
					}
					break;
				case weapon::State::OutOfAmmo:
					if (curTime >= wpn.stateEnd)
					{
						beginIdle(curTime, wpn, animator);
					}
					break;
				case weapon::State::Reloading:
					if (curTime >= wpn.stateEnd)
					{
						// add the ammo post reload.
						if (wpn.ownerEnt != INVALID_ENT_ID && pReg_->has<Inventory>(wpn.ownerEnt)) 
						{
							auto& inv = pReg_->get<Inventory>(wpn.ownerEnt);
						
							int32_t avaliableAmmo = inv.ammo; // TODO: use ammo types
							int32_t clipSize = wpn.pWeaponDef->getAmmoSlot(weapon::AmmoSlot::ClipSize);
							int32_t clipSpace = clipSize - wpn.ammoInClip;
							int32_t ammoToAdd = core::Min(clipSpace, core::Min(clipSize, avaliableAmmo));

							if (ammoToAdd != 0) 
							{
								wpn.ammoInClip += ammoToAdd;
								inv.ammo -= ammoToAdd;
							}
						}

						wpn.reload = false;
						beginIdle(curTime, wpn, animator);
					}
					break;
				case weapon::State::Idle:
				{
					// if we are idle we can trainsition to:
					// fire, drop, reload
					// the weapon can be put into fire mode via various things.
					// like the player could start the attack.
					// and the animation / sounds are handled here.
					if (wpn.stateEnd + 3000_ms < curTime)
					{
						beginLower(curTime, wpn, animator);
						continue;
					}

					if (wpn.reload)
					{
						wpn.reload = false;
						beginReload(curTime, wpn, animator);
					}
					else if (wpn.attack)
					{
						beginAttack(curTime, wpn, animator);
					}

				}
				break;

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

		if (!wpn.stateFlags.IsSet(weapon::StateFlag::HasRaised))
		{
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

	void WeaponSystem::beginAttack(core::TimeVal curTime, Weapon& wpn, Animator& animator)
	{
		// need to check ammo.
		if (wpn.ammoInClip == 0)
		{
			X_LOG0("Weapon", "Clip empty");

			if (!beginReload(curTime, wpn, animator))
			{
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


		trainsitionToState(wpn, animator, anim, weapon::State::Fire, curTime, fireTime);
	}

	bool WeaponSystem::beginReload(core::TimeVal curTime, Weapon& wpn, Animator& animator)
	{
		// can we reload?
		// check if we have ammo, but don't add it yet.
		{
			if (wpn.ownerEnt == INVALID_ENT_ID || !pReg_->has<Inventory>(wpn.ownerEnt)) {
				return false;
			}

			auto& inv = pReg_->get<Inventory>(wpn.ownerEnt);
			int32_t avaliableAmmo = inv.ammo; // TODO: use ammo types

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
		X_LOG0("Weapon", "State change %s -> %s", weapon::State::ToString(wpn.state), weapon::State::ToString(newState));

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

}

X_NAMESPACE_END