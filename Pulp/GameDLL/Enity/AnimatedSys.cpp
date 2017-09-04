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

			auto bones = an.pAnimator->getBoneMatrices();


			static float counter = 0.f;
			static bool forward = true;

			if (forward)
			{
				counter += 0.1f;

				if (counter >= 0.f)
				{
					forward = false;
				}
			}
			else
			{
				counter -= 0.1f;

				if (counter < -20.f)
				{
					forward = true;
				}
			}

			bones[1].setTranslate(Vec3f(0.f, 0.f, counter / 4.f));
			bones[2] = Matrix44f::createRotation(Vec3f(0.f, 1.f, 0.f), counter / 100.f);

			// update the matrix we render with.
			// cus meow.
			p3DWorld->setBonesMatrix(rendEnt.pRenderEnt, bones.data(), bones.size());
		}
	}


}

X_NAMESPACE_END