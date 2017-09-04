#include "stdafx.h"
#include "AnimatedSys.h"

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IWorld3D.h>
#include <IAnimManager.h>


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

	void AnimatedSystem::update(core::FrameTimeData& time, EnitiyRegister& reg, engine::IWorld3D* p3DWorld)
	{
		X_UNUSED(time, reg, p3DWorld);

		static bool once = true;

		if(once)
		{
			once = false;

			auto* panimMan = gEnv->p3DEngine->getAnimManager();
			auto* pAnim = panimMan->loadAnim("test/smooth_bind_01_pump");


			auto view = reg.view<Animated, Mesh, TransForm>();
			for (auto entity : view)
			{
				auto& an = reg.get<Animated>(entity);


				an.pAnimator->playAnim(pAnim, time.ellapsed[core::Timer::GAME], 500_ms);
			}
		}

		auto view = reg.view<Animated, Mesh, TransForm, MeshRenderer>();
		for (auto entity : view)
		{
			auto& an = reg.get<Animated>(entity);
			auto& rendEnt = reg.get<MeshRenderer>(entity);

			an.pAnimator->createFrame(time.ellapsed[core::Timer::GAME]);

			auto& bones = an.pAnimator->getBoneMatrices();

			// update the matrix we render with.
			// cus meow.
			p3DWorld->setBonesMatrix(rendEnt.pRenderEnt, bones.data(), bones.size());
		}
	}


}

X_NAMESPACE_END