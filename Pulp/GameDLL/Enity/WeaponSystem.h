#pragma once

#include <IAnimManager.h>


X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
)

X_NAMESPACE_BEGIN(game)

namespace entity
{

	class WeaponSystem
	{
	public:
		WeaponSystem();

		bool init(void);
		void update(core::FrameTimeData& time, EnitiyRegister& reg);

	private:
		void beginRaise(core::TimeVal curTime, Weapon& wpn, Animator& animator);
		void beginLower(core::TimeVal curTime, Weapon& wpn, Animator& animator);
		void beginIdle(core::TimeVal curTime, Weapon& wpn, Animator& animator);
		void beginAttack(core::TimeVal curTime, Weapon& wpn, Animator& animator);
		bool beginReload(core::TimeVal curTime, Weapon& wpn, Animator& animator);

		void trainsitionToState(Weapon& wpn, Animator& animator, weapon::AnimSlot::Enum anim, weapon::State::Enum newState,
			core::TimeVal curTime, core::TimeVal transTime);
	private:

		EnitiyRegister* pReg_;
		anim::IAnimManager* pAnimManager_;

	};

}

X_NAMESPACE_END