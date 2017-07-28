#include "stdafx.h"
#include "EnitiySystem.h"
#include "UserCmds\UserCmdMan.h"
#include "Vars\GameVars.h"

#include <String\Json.h>
#include <Hashing\Fnva1Hash.h>

#include <IFrameData.h>

X_NAMESPACE_BEGIN(game)



namespace entity
{


	// -----------------------------------------------------------

	EnititySystem::EnititySystem(GameVars& vars, core::MemoryArenaBase* arena) :
		reg_(arena),
		vars_(vars),
		playerSys_(vars.player)
	{
		pPhysics_ = nullptr;
		pPhysScene_ = nullptr;
	}


	bool EnititySystem::init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene)
	{
		pPhysics_ = X_ASSERT_NOT_NULL(pPhysics);
		pPhysScene_ = X_ASSERT_NOT_NULL(pPhysScene);

		if (!playerSys_.init(pPhysScene)) {
			return false;
		}

		if (!cameraSys_.init()) {
			return false;
		}

		for (uint32_t i = 0; i < MAX_PLAYERS; i++) {
			auto id = createEnt();
			if (id != i) {
				X_ASSERT_UNREACHABLE();
				return false;
			}
		}

		return true;
	}


	void EnititySystem::update(core::FrameData& frame, UserCmdMan& userCmdMan)
	{
		// process input.
	//	inputSys_.update(frame.timeInfo, reg_, pPhysScene_);

		// process the userCmd for the current player.
		EntityId id = 0;

		auto& userCmd = userCmdMan.getUserCmdForPlayer(id);
		auto unread = userCmdMan.getNumUnreadFrames(id);
		X_UNUSED(unread);

		X_LOG0_EVERY_N(60, "Goat", "Unread %i", unread);

#if 1
		playerSys_.runUserCmdForPlayer(frame.timeInfo, reg_, userCmd, id);


#else
		{
			const float speed = 250.f;
			const float gravity = -100.f;
			const float timeDelta = frame.timeInfo.deltas[core::ITimer::Timer::GAME].GetSeconds();
			const float timeScale = speed * frame.timeInfo.deltas[core::ITimer::Timer::GAME].GetSeconds();

			auto upDisp = Vec3f::zAxis();
			upDisp *= gravity * timeDelta;

			Vec3f displacement;

			if (userCmd.moveForwrd != 0)
			{
				displacement.y += userCmd.moveForwrd * timeDelta * speed;
			}
			if (userCmd.moveRight != 0)
			{
				displacement.x += userCmd.moveRight * timeDelta * speed;
			}

			displacement += upDisp;

			if (reg_.has<Player, CharacterController>(id))
			{
				auto& con = reg_.get<CharacterController>(id);
				auto& trans = reg_.get<TransForm>(id);
				auto& player = reg_.get<Player>(id);


				physics::ScopedLock lock(pPhysScene_, true);

				auto flags = con.pController->move(displacement, 0.f, timeDelta);
				if (flags.IsAnySet())
				{

				}


				auto& a = userCmd.angles;
				trans.pos = con.pController->getPosition();


				// for the position we want to just add the delta.
				// which is in axis angles.
				// but then we also want to clamp the view.
				
				
				//viewAngles.pitch = std::min(viewAngles.pitch, pm_maxviewpitch.GetFloat() * restrict);
				//viewAngles.pitch = std::max(viewAngles.pitch, pm_minviewpitch.GetFloat() * restrict);

				trans.quat = a.toQuat();

				player.cameraOrigin = trans.pos + player.eyeOffset;
				player.cameraAxis = a; // trans.quat.getEuler();
			}
		}
#endif

		// update the cameras.
		cameraSys_.update(frame, reg_, pPhysScene_);

	}


	EntityId EnititySystem::createEnt(void)
	{
		auto ent = reg_.create<TransForm>();

		return ent;
	}

	void EnititySystem::makePlayer(EntityId id)
	{
		// auto trans = reg_.assign<TransForm>(id);
		auto& player = reg_.assign<Player>(id);
		X_UNUSED(player);
//		player.eyeOffset = Vec3f(0, 0, 50.f);
//		player.cameraOrigin = Vec3f(0, 0, 50.f);
//		player.cameraAxis = Anglesf(0, 0, 0.f);

		// temp.
		cameraSys_.setActiveEnt(id);
	}

	bool EnititySystem::addController(EntityId id)
	{
		auto& trans = reg_.get<TransForm>(id);
		auto& con = reg_.assign<CharacterController>(id);

		physics::CapsuleControllerDesc desc;
		desc.radius = 20.f;
		desc.height = vars_.player.normalHeight_;
		desc.climbingMode = physics::CapsuleControllerDesc::ClimbingMode::Easy;
		desc.material = pPhysics_->getDefaultMaterial();
		desc.position = trans.pos;
		desc.upDirection = Vec3f::zAxis();
		desc.maxJumpHeight = 50.f;

		con.pController = pPhysScene_->createCharacterController(desc);

		if (con.pController == nullptr)
		{
			reg_.remove<CharacterController>(id);
			return false;
		}

		return true;
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

		if (d.HasMember("trigger"))
		{
			auto arr = d["trigger"].GetArray();

			if (!parseTriggers(arr))
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
			auto ent = reg_.create<TransForm, PhysicsComponent>();
			auto& trans = reg_.get<TransForm>(ent);
	//		auto& phys = reg_.get<PhysicsComponent>(ent);

			auto& kvps = *it;

			const char* pOrigin = kvps["origin"].GetString();
			const char* pModel = kvps["model"].GetString();

			if (sscanf_s(pOrigin, "%f %f %f", &trans.pos.x, &trans.pos.y, &trans.pos.z) != 3) {
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
			auto ent = reg_.create<TransForm, ScriptName>();
			auto& trans = reg_.get<TransForm>(ent);
			auto& name = reg_.get<ScriptName>(ent);

			auto& kvps = *it;

			const char* pOrigin = kvps["origin"].GetString();
			const char* pTargetName = kvps["targetname"].GetString();

			auto& pos = trans.pos;
			if (sscanf_s(pOrigin, "%f %f %f", &pos.x, &pos.y, &pos.z) != 3) {
				return false;
			}

			name.pName = pTargetName;
		}

		return true;
	}

	bool EnititySystem::parseTriggers(core::json::Value::Array val)
	{
		X_UNUSED(val);

		return true;
	}

} // namespace entity

X_NAMESPACE_END