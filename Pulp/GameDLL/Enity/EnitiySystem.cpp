#include "stdafx.h"
#include "EnitiySystem.h"

#include <String\Json.h>
#include <Hashing\Fnva1Hash.h>

#include <IFrameData.h>

X_NAMESPACE_BEGIN(game)



namespace entity
{


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
		cameraSys_.update(frame, ecs_, pPhysScene_);

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

	bool EnititySystem::parseTriggers(core::json::Value::Array val)
	{
		X_UNUSED(val);

		return true;
	}

} // namespace entity

X_NAMESPACE_END