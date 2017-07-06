#include "stdafx.h"
#include "InputSys.h"

#include <IFrameData.h>


X_NAMESPACE_BEGIN(game)

namespace entity
{

	bool InputSystem::init(void)
	{

		gEnv->pInput->AddEventListener(this);

		return true;
	}


	void InputSystem::update(core::FrameTimeData& timeInfo, EnitiyRegister& reg, physics::IScene* pPhysScene)
	{
		// we need to pass input onto something.
		// - like walking up to somthing in the world that accepts input should swtich to that ent getting input.
		// - so maybe with have a input compnent that syas ents are intrested in input.
		//
		// - For base case we have a player that accepts direction commapnds say via keyboard input.
		//   But the player don't rotate.
		// how would i do this if all in one ent.
		// i would need like a active ent that i would target the input at.
		// but here since it's segmented many ents can register for it.

		// lets just get the physics capsule moving.
		const float speed = 250.f;
		const float gravity = -100.f;
		const float timeDelta = timeInfo.deltas[core::ITimer::Timer::GAME].GetSeconds();
		const float timeScale = speed * timeInfo.deltas[core::ITimer::Timer::GAME].GetSeconds();

		auto upDisp = Vec3f::zAxis();
		upDisp *= gravity * timeDelta;

		auto view = reg.view<CharacterController, TransForm>();
		for (auto entity : view)
		{
			auto& con = reg.get<CharacterController>(entity);
			auto& trans = reg.get<TransForm>(entity);

			// Quatf angle = trans.trans.quat;
			Vec3f displacement;
			Vec3f angle = trans.trans.quat.getEuler();

			for (const auto& e : inputEvents_)
			{
				Vec3f posDelta;

				// rotate.
				switch (e.keyId)
				{
					case input::KeyId::MOUSE_X:
					//	angle *= Quatf(0, ::toRadians(-(e.value * 0.2f)), 0);
						angle.z += -(e.value * 0.002f);

						continue;
					case input::KeyId::MOUSE_Y:
					//	angle *= Quatf(0,0,::toRadians(-(e.value * 0.2f)));
						angle.x += -(e.value * 0.002f);

						continue;
					default:
						break;
				}

				float scale = 1.f;
				if (e.modifiers.IsSet(input::InputEvent::ModiferType::LSHIFT)) {
					scale = 2.f;
				}

				scale *= timeScale;

				switch (e.keyId)
				{
					// forwards.
					case input::KeyId::W:
						posDelta.y = scale;
						break;
						// backwards
					case input::KeyId::A:
						posDelta.x = -scale;
						break;

						// Left
					case input::KeyId::S:
						posDelta.y = -scale;
						break;
						// right
					case input::KeyId::D:
						posDelta.x = scale;
						break;

						// up / Down
					case input::KeyId::Q:
						posDelta.z = -scale;
						break;
					case input::KeyId::E:
						posDelta.z = scale;
						break;

					default:
						continue;
				}
#if 1
			//	Vec3f eu = angle.getEuler();
				Matrix33f goat = Matrix33f::createRotation(angle);

				displacement += (goat * posDelta);
#else

				displacement += (angle * posDelta);
#endif
			}

			displacement += upDisp;

			if (displacement != Vec3f::zero())
			{
				physics::ScopedLock lock(pPhysScene, true);

				auto flags = con.pController->move(displacement, 0.f, timeDelta);
				if (flags.IsAnySet())
				{

				}


				trans.trans.pos = con.pController->getPosition();
				trans.trans.quat = Quatf(angle.x, angle.y, angle.z);
			}
		}


		inputEvents_.clear();

	}


	void InputSystem::processInput(core::FrameTimeData& timeInfo)
	{
		X_UNUSED(timeInfo);


	}


	bool InputSystem::OnInputEvent(const input::InputEvent& event)
	{
		// theses event have pointers to symbols that will change in next input poll
		// but we don't use any of the data from symbol.
		inputEvents_.emplace_back(event);
		return false;
	}

	bool InputSystem::OnInputEventChar(const input::InputEvent& event)
	{
		X_UNUSED(event);
		return false;
	}



} // namespace entity

X_NAMESPACE_END