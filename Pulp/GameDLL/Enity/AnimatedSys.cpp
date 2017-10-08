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

	void AnimatedSystem::update(core::FrameTimeData& time, EnitiyRegister& reg, engine::IWorld3D* p3DWorld, physics::IScene* pPhysScene)
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

		auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::MISC3D);

		auto ellapsedTime = time.ellapsed[core::Timer::GAME];

		{
			auto view = reg.view<Attached, TransForm>();
			for (auto entity : view)
			{
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

				if (att.parentBone != model::INVALID_BONE_HANDLE)
				{
					if (!reg.has<Animator>(parent)) {
						continue;
					}

					auto& an = reg.get<Animator>(parent);
					Vec3f bonePos;
					Matrix33f boneAxis;

					if (an.pAnimator->getBoneTransform(att.parentBone, ellapsedTime, bonePos, boneAxis))
					{
						trans.pos = parTrans.pos + bonePos * parTrans.quat;
						trans.quat = Quatf(boneAxis) * trans.quat;
					}
				}
			
				if (reg.has<MeshRenderer>(entity))
				{
					auto& rendEnt = reg.get<MeshRenderer>(entity);
					p3DWorld->updateRenderEnt(rendEnt.pRenderEnt, trans);
				}
			}
		}

		{
			auto view = reg.view<Rotator, TransForm>();
			for (auto entity : view)
			{
				auto& trans = reg.get<TransForm>(entity);
				auto& rot = reg.get<Rotator>(entity);

				trans.quat *= Quatf(rot.axis, ::toRadians(rot.speed));
			
				if (reg.has<MeshRenderer>(entity))
				{
					auto& rendEnt = reg.get<MeshRenderer>(entity);
					p3DWorld->updateRenderEnt(rendEnt.pRenderEnt, trans);
				}
				if (reg.has<MeshCollider>(entity))
				{
					auto& col = reg.get<MeshCollider>(entity);
					pPhysScene->setKinematicTarget(col.actor, trans);
				}
			}
		}


		auto view = reg.view<Animator, Mesh, MeshRenderer, TransForm>();
		for (auto entity : view)
		{
			auto& trans = reg.get<TransForm>(entity);
			auto& an = reg.get<Animator>(entity);
			auto& rendEnt = reg.get<MeshRenderer>(entity);

			X_UNUSED(pPrim, trans);
		//	an.pAnimator->renderInfo(ellapsedTime, trans.pos + Vec3f(0.f, 0.f, -20.f), rotation, pPrim);

			if (!an.pAnimator->isAnimating(ellapsedTime)) {
				continue;
			}

			if (an.pAnimator->createFrame(ellapsedTime))
			{
			
			}
			
			auto& bones = an.pAnimator->getBoneMatrices();
			
			// the frame for the current time may have already been made.
			// so create frame returns false, but we still need to update the bones.
			p3DWorld->setBonesMatrix(rendEnt.pRenderEnt, bones.data(), bones.size());
		}
	}


}

X_NAMESPACE_END