#include "stdafx.h"
#include "EnitiySystem.h"

#include <String\Json.h>
#include <Hashing\Fnva1Hash.h>

#include <IFrameData.h>
#include <IRender.h> // temp

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

			Quatf angle = trans.trans.quat;
			Vec3f displacement;

			for (const auto& e : inputEvents_)
			{
				Vec3f posDelta;

				// rotate.
				switch (e.keyId)
				{
					case input::KeyId::MOUSE_Y:
						angle *= Quatf(0,0,::toRadians(-(e.value * 0.002f)));
						continue;
					case input::KeyId::MOUSE_X:
						angle *= Quatf (0,::toRadians( -(e.value * 0.002f)),0);
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
				Vec3f eu = angle.getEuler();
				Matrix33f goat = Matrix33f::createRotation(eu);

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
				trans.trans.quat = angle;
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

	// -----------------------------------------------------------

	CameraSystem::CameraSystem() :
		activeEnt_(EnitiyRegister::INVALID_ID)
	{

	}

	CameraSystem::~CameraSystem()
	{

	}

	bool CameraSystem::init(void)
	{
		auto deimension = gEnv->pRender->getDisplayRes();

		cam_.SetFrustum(deimension.x, deimension.y, DEFAULT_FOV, 1.f, 2048.f);


		return true;
	}

	void CameraSystem::update(core::FrameData& frame, EnitiyRegister& reg)
	{
		if (reg.isValid(activeEnt_))
		{
			if (reg.has<TransForm>(activeEnt_))
			{
				auto& trans = reg.get<TransForm>(activeEnt_);

				cameraPos_ = trans.trans.pos;
				cameraPos_ += Vec3f(0,0,20.f);
				cameraAngle_ = trans.trans.quat.getEuler();
			}
			else if (reg.has<Position>(activeEnt_))
			{
				auto& pos = reg.get<Position>(activeEnt_);

				cameraPos_ = pos.pos;
			}
			else
			{
				X_ASSERT_UNREACHABLE();
			}
		}

		cam_.setAngles(cameraAngle_);
		cam_.setPosition(cameraPos_);

		// Pro
		frame.view.cam = cam_;
		frame.view.projMatrix = cam_.getProjectionMatrix();
		frame.view.viewMatrix = cam_.getViewMatrix();
		frame.view.viewProjMatrix = frame.view.viewMatrix * frame.view.projMatrix;
		frame.view.viewProjInvMatrix = frame.view.viewProjMatrix.inverted();
	}

	void CameraSystem::setActiveEnt(EntityId entId)
	{
		activeEnt_ = entId;
	}


	// -----------------------------------------------------------

	EnititySystem::EnititySystem(core::MemoryArenaBase* arena) :
		ecs_(arena)
	{
		pPhysics_ = nullptr;
		pPhysScene_ = nullptr;
	}

	bool EnititySystem::init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene)
	{
		pPhysics_ = pPhysics;
		pPhysScene_ = pPhysScene;

		if (!inputSys_.init()) {
			return false;
		}

		if (!cameraSys_.init()) {
			return false;
		}

		return true;
	}


	void EnititySystem::update(core::FrameData& frame)
	{
		// process input.
		inputSys_.update(frame.timeInfo, ecs_, pPhysScene_);

		// update the cameras.
		cameraSys_.update(frame, ecs_);

	}



	EnititySystem::EntityId EnititySystem::createPlayer(const Vec3f& origin)
	{
		auto ent = ecs_.create<TransForm, CharacterController>();
		auto& trans = ecs_.get<TransForm>(ent);
		auto& con = ecs_.get<CharacterController>(ent);

		physics::CapsuleControllerDesc desc;
		desc.radius = 20.f;
		desc.height = 1.f;
		desc.climbingMode = physics::CapsuleControllerDesc::ClimbingMode::Easy;
		desc.material = pPhysics_->getDefaultMaterial();
		desc.position = origin;
		desc.upDirection = Vec3f::zAxis();

		trans.trans.pos = origin;
		trans.trans.quat = Quatf::identity();
		con.pController = pPhysScene_->createCharacterController(desc);

		if (con.pController == nullptr)
		{
			ecs_.destroy(ent);
			return EnitiyRegister::INVALID_ID;
		}

		cameraSys_.setActiveEnt(ent);

		return ent;
	}

	bool EnititySystem::loadEntites(const char* pJsonBegin, const char* pJsonEnd)
	{
		X_UNUSED(pJsonEnd);

		core::json::Document d;
		if (d.ParseInsitu(const_cast<char*>(pJsonBegin)).HasParseError()) {
			auto err = d.GetParseError();
			X_ERROR("Ents", "Failed to parse ent desc: %i", err);
			return false;
		}

		if (d.GetType() != core::json::Type::kObjectType) {
			return false;
		}

		if (d.HasMember("misc_model"))
		{
			auto arr = d["misc_model"].GetArray();

			if (!parseMiscModels(arr))
			{
				return false;
			}
		}

		if (d.HasMember("script_origin"))
		{
			auto arr = d["script_origin"].GetArray();

			if (!parseScriptOrigins(arr))
			{
				return false;
			}
		}


		return true;
	}

	bool EnititySystem::parseMiscModels(core::json::Value::Array arr)
	{
		for (auto it = arr.begin(); it != arr.end(); ++it)
		{
			auto ent = ecs_.create<Position, PhysicsComponent>();
			auto& pos = ecs_.get<Position>(ent);
	//		auto& phys = ecs_.get<PhysicsComponent>(ent);

			auto& kvps = *it;

			const char* pOrigin = kvps["origin"].GetString();
			const char* pModel = kvps["model"].GetString();

			if (sscanf_s(pOrigin, "%f %f %f", &pos.pos.x, &pos.pos.y, &pos.pos.z) != 3) {
				return false;
			}

			// we know the model name but it's not loaded yet.
			// so we dunno it's phyics.
			// only once it's loaded can we spawn a instance of the physics.
			// this is just data tho, so we should add this to a list of pending physics loads?
			// then a system can handle the loading of the models and assigning physics actors.
			// i think i will force all models to be loaded, but they don't have to be backed by render meshes.
			// thinks like physics triggers need some logic to create a trigger and we need to handle when it's triggered.
			X_UNUSED(pModel);

			// pPhysics_
		}

		return true;
	}


	bool EnititySystem::parseScriptOrigins(core::json::Value::Array arr)
	{
		for (auto it = arr.begin(); it != arr.end(); ++it)
		{
			auto ent = ecs_.create<Position, ScriptName>();
			auto& pos = ecs_.get<Position>(ent);
			auto& name = ecs_.get<ScriptName>(ent);

			auto& kvps = *it;

			const char* pOrigin = kvps["origin"].GetString();
			const char* pTargetName = kvps["targetname"].GetString();

			if (sscanf_s(pOrigin, "%f %f %f", &pos.pos.x, &pos.pos.y, &pos.pos.z) != 3) {
				return false;
			}

			name.pName = pTargetName;
		}

		return true;
	}


} // namespace entity

X_NAMESPACE_END