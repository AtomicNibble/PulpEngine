#include "stdafx.h"
#include "EnitiySystem.h"
#include "UserCmds\UserCmdMan.h"
#include "Vars\GameVars.h"

#include <String\Json.h>
#include <Hashing\Fnva1Hash.h>

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IWorld3D.h>

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
		p3DWorld_ = nullptr;
		pModelManager_ = nullptr;
	}


	bool EnititySystem::init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld)
	{
		pPhysics_ = X_ASSERT_NOT_NULL(pPhysics);
		pPhysScene_ = X_ASSERT_NOT_NULL(pPhysScene);
		p3DWorld_ = X_ASSERT_NOT_NULL(p3DWorld);
		pModelManager_ = gEnv->p3DEngine->getModelManager();

		if (!pModelManager_) {
			return false;
		}

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


		playerSys_.runUserCmdForPlayer(frame.timeInfo, reg_, userCmd, id);


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
		auto& hp = reg_.assign<Health>(id);
		auto& rend = reg_.assign<RenderComponent>(id);

		X_UNUSED(player);

//		player.eyeOffset = Vec3f(0, 0, 50.f);
//		player.cameraOrigin = Vec3f(0, 0, 50.f);
//		player.cameraAxis = Anglesf(0, 0, 0.f);

		hp.hp = 100;

		engine::RenderEntDesc entDsc;
		entDsc.pModel = pModelManager_->loadModel("default/default");
		entDsc.trans.pos = Vec3f(-90, 0, 10);

		rend.pRenderEnt = p3DWorld_->addRenderEnt(entDsc);
		X_ASSERT_NOT_NULL(rend.pRenderEnt);


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
#if 1
		X_UNUSED(arr);
		return true;
#else
		for (auto it = arr.begin(); it != arr.end(); ++it)
		{
			auto ent = reg_.create<TransForm, PhysicsComponent, RenderComponent>();
			auto& trans = reg_.get<TransForm>(ent);
			auto& phys = reg_.get<PhysicsComponent>(ent);
			auto& rend = reg_.get<RenderComponent>(ent);

			X_UNUSED(phys, rend);

			auto& kvps = *it;

			const char* pOrigin = kvps["origin"].GetString();
			const char* pModelName = kvps["model"].GetString();

			if (sscanf_s(pOrigin, "%f %f %f", &trans.pos.x, &trans.pos.y, &trans.pos.z) != 3) {
				return false;
			}

			rend.pModel = pModelManager_->loadModel(pModelName);
		}

		return true;
#endif
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