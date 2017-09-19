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


			auto view = reg.view<Animator, Mesh, TransForm>();
			for (auto entity : view)
			{
				auto& an = reg.get<Animator>(entity);


				an.pAnimator->playAnim(pAnim, time.ellapsed[core::Timer::GAME], 500_ms);
			}
		}

		Matrix33f rotation;
		rotation.rotate(Vec3f::xAxis(), ::toRadians(90.f));
		rotation.rotate(Vec3f::yAxis(), ::toRadians(180.f));
		rotation.rotate(Vec3f::zAxis(), ::toRadians(180.f));
		// rotation.rotate(Vec3f::yAxis(), ::toRadians(90.f));
		// rotation.rotate(Vec3f::yAxis(), ::toRadians(90.f));

		auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::MISC3D);

		auto view = reg.view<Animator, Mesh, MeshRenderer, TransForm>();
		for (auto entity : view)
		{
			auto& trans = reg.get<TransForm>(entity);
			auto& an = reg.get<Animator>(entity);
			auto& rendEnt = reg.get<MeshRenderer>(entity);

			an.pAnimator->createFrame(time.ellapsed[core::Timer::GAME]);
			an.pAnimator->renderInfo(time.ellapsed[core::Timer::GAME], trans.pos + Vec3f(0.f, 0.f, -15.f), rotation, pPrim);

			auto& bones = an.pAnimator->getBoneMatrices();

			// update the matrix we render with.
			// cus meow.
			p3DWorld->setBonesMatrix(rendEnt.pRenderEnt, bones.data(), bones.size());
		}
	}


}

X_NAMESPACE_END