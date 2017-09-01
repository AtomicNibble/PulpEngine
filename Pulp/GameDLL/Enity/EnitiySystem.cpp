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
		arena_(arena),
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

		if (!soundSys_.init()) {
			return false;
		}

		for (uint32_t i = 0; i < MAX_PLAYERS; i++) {
			auto id = createEnt();
			if (id != i) {
				X_ASSERT_UNREACHABLE();
				return false;
			}
		}

		// need to build translators for all the components.
		if (!createTranslatours()) {
			return false;
		}

		return true;
	}


	bool EnititySystem::createTranslatours(void)
	{

#if 0
		const char* pDoc = R"(
{
	"ents" : [
		{
			"origin" : {"x": 128, "y": 128, "z": 25},
			"angles" : {"p": -2.618721, "y": 1.028581, "r": 25.0},

			"health" : {
				"hp": 100,
				"max": 100
			},
			"SoundObject" : {
			}
		}
	]
}
			)";


		loadEntites2(pDoc, pDoc + sizeof(pDoc));

#endif

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

		soundSys_.update(frame, reg_);
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
		desc.maxJumpHeight = vars_.player.jumpHeight_;

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


		return true;
	}

	bool EnititySystem::loadEntites2(const char* pJsonBegin, const char* pJsonEnd)
	{
		X_UNUSED(pJsonEnd);

		core::json::Document d;
		if (d.ParseInsitu(const_cast<char*>(pJsonBegin)).HasParseError()) {
			auto err = d.GetParseError();
			X_ERROR("Ents", "Failed to parse ent desc: %i", err);
			return false;
		}


		if (d.GetType() != core::json::Type::kObjectType) {
			X_ERROR("Ents", "Unexpected type");
			return false;
		}

		for (auto it = d.MemberBegin(); it != d.MemberEnd(); ++it)
		{
			auto& v = *it;

			if (v.name == "ents")
			{
				if (v.value.GetType() != core::json::Type::kArrayType) {
					X_ERROR("Ents", "Ent data must be array");
					return false;
				}

				auto ents = v.value.GetArray();
				for (auto& ent : ents)
				{
					if (!parseEntDesc(ent)) {
						X_ERROR("Ents", "Unexpected type");
						return false;
					}
				}
			}
			else
			{
				X_ERROR("Ents", "Unknown ent data member: \"%s\"", v.name.GetString());
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
			auto ent = reg_.create<TransForm, SoundObject>();
			auto& trans = reg_.get<TransForm>(ent);
			auto& snd = reg_.get<SoundObject>(ent);

			auto& kvps = *it;

			const char* pOrigin = kvps["origin"].GetString();

			auto& pos = trans.pos;
			if (sscanf_s(pOrigin, "%f %f %f", &pos.x, &pos.y, &pos.z) != 3) {
				return false;
			}

		//	name.pName = pTargetName;

			snd.handle = gEnv->pSound->registerObject(trans, "Goat");
			 gEnv->pSound->setOcclusionType(snd.handle, sound::OcclusionType::SingleRay);

			if (kvps.HasMember("sound_evt")) {
				const char* pEvt = kvps["sound_evt"].GetString();

				gEnv->pSound->postEvent(pEvt, snd.handle);
			}
		}

		return true;
	}


	template<typename CompnentT>
	bool EnititySystem::parseComponent(DataTranslator<CompnentT>& translator, CompnentT& comp, const core::json::Value& compDesc)
	{
		if (compDesc.GetType() != core::json::Type::kObjectType) {
			X_ERROR("Ents", "Component description must be a object");
			return false;
		}

		for (core::json::Value::ConstMemberIterator it = compDesc.MemberBegin(); it != compDesc.MemberEnd(); ++it)
		{
			const auto& name = it->name;
			const auto& val = it->value;

			core::StrHash nameHash(name.GetString(), name.GetStringLength());

			switch (val.GetType())
			{
				case core::json::Type::kNumberType:

					if (val.IsInt()) {
						translator.AssignInt(comp, nameHash, val.GetInt());
					}
					else if (val.IsBool()) {
						translator.AssignBool(comp, nameHash, val.GetBool());
					}
					else if (val.IsFloat()) {
						translator.AssignFloat(comp, nameHash, val.GetFloat());
					}
					else {
						X_ERROR("Ent", "Unknown component numeric type: %" PRIi32, val.GetType());
						return false;
					}

					break;
				case core::json::Type::kStringType:
					translator.AssignString(comp, nameHash, val.GetString());
					break;

				case core::json::Type::kObjectType:
				{
					if (!val.HasMember("x") || !val.HasMember("y") || !val.HasMember("z")) {
						X_ERROR("Ents", "Invalid vec3");
						return false;
					}

					Vec3f vec(
						val["x"].GetFloat(),
						val["y"].GetFloat(),
						val["z"].GetFloat()
					);

					translator.AssignVec3(comp, nameHash, vec);
				}
					break;

				default:
					X_ERROR("Ent", "Unknown component type: %" PRIi32, val.GetType());
					return false;
			}
		}

		return true;
	}

	bool EnititySystem::parseEntDesc(core::json::Value& entDesc)
	{
		using namespace core::Hash::Fnv1Literals;

		// so we need a way to write these out also.
		// so that the editor can show them and dispaly required props.
		DataTranslator<Health> dtHealth(arena_);
		ADD_TRANS_MEMBER(dtHealth, hp);
		ADD_TRANS_MEMBER(dtHealth, max);

		DataTranslator<Mesh> dtMesh(arena_);
		ADD_TRANS_MEMBER(dtMesh, name);

		DataTranslator<SoundObject> dtSoundObj(arena_);
		ADD_TRANS_MEMBER(dtSoundObj, offset);
		ADD_TRANS_MEMBER(dtSoundObj, occType);

		if (entDesc.GetType() != core::json::Type::kObjectType) {
			X_ERROR("Ents", "Ent description must be a object");
			return false;
		}

		EntityId ent = reg_.create<TransForm>();
		auto& trans = reg_.get<TransForm>(ent);


		for (auto it = entDesc.MemberBegin(); it != entDesc.MemberEnd(); ++it)
		{
			const auto& name = it->name;
			const auto& value = it->value;

			switch (core::Hash::Fnv1aHash(name.GetString(), name.GetStringLength()))
			{
				case "Name"_fnv1a:
				{
					auto& hp = reg_.assign<Health>(ent);
					if (!parseComponent(dtHealth, hp, value)) {
						return false;
					}
					break;
				}
				case "Health"_fnv1a:
				{
					auto& hp = reg_.assign<Health>(ent);
					if (!parseComponent(dtHealth, hp, value)) {
						return false;
					}
					break;
				}
				case "SoundObject"_fnv1a:
				{
					auto& snd = reg_.assign<SoundObject>(ent);
					if (!parseComponent(dtSoundObj, snd, value)) {
						return false;
					}

#if X_SOUND_ENABLE_DEBUG_NAMES
					if (reg_.has<EntName>(ent))
					{
						auto& entName = reg_.get<EntName>(ent);

						snd.handle = gEnv->pSound->registerObject(trans, entName.name.c_str());
					}
					else
#endif // !X_SOUND_ENABLE_DEBUG_NAMES
					{
						snd.handle = gEnv->pSound->registerObject(trans);
					}

					if (snd.occType.isNotEmpty())
					{
						sound::OcclusionType::Enum occ = sound::OcclusionType::None;

						switch (core::Hash::Fnv1aHash(snd.occType.c_str(), snd.occType.length()))
						{
							case "None"_fnv1a:
								occ = sound::OcclusionType::None;
								break;
							case "SingleRay"_fnv1a:
								occ = sound::OcclusionType::SingleRay;
								break;
							case "MultiRay"_fnv1a:
								occ = sound::OcclusionType::MultiRay;
								break;
							default:
								X_ERROR("Ent", "Invalid occlusion type: \"%s\"", snd.occType.c_str());
								break;
						}

						if (occ != sound::OcclusionType::None)
						{
							gEnv->pSound->setOcclusionType(snd.handle, occ);
						}
					}

					break;
				}

				case "MeshRenderer"_fnv1a:
				{
					if (!reg_.has<Mesh>(ent)) {
						X_ERROR("Ents", "MeshRenderer requires a Mesh component");
						return false;
					}

					auto& mesh = reg_.get<Mesh>(ent);
					auto& meshRend = reg_.assign<MeshRenderer>(ent);

					engine::RenderEntDesc entDsc;
					entDsc.pModel = mesh.pModel;
					entDsc.trans = trans;

					meshRend.pRenderEnt = p3DWorld_->addRenderEnt(entDsc);
					break;
				}

				case "MeshCollider"_fnv1a:
				{
					if (!reg_.has<Mesh>(ent)) {
						X_ERROR("Ents", "MeshCollider requires a Mesh component");
						return false;
					}

					// we just assign the component
					// then later before we finished loading we iterate these waiting for the models to 
					// finish loading and then create the physics.
					reg_.assign<MeshCollider>(ent);
					break;
				}

				case "Mesh"_fnv1a:
				{
					auto& mesh = reg_.assign<Mesh>(ent);
					if (!parseComponent(dtMesh, mesh, value)) {
						return false;
					}
					if (mesh.name.isEmpty()) {
						X_ERROR("Ents", "Mesh has empty name");
						return false;
					}

					mesh.pModel = pModelManager_->loadModel(mesh.name.c_str());
					break;
				}



				// loosy goosy
				case "origin"_fnv1a:
				{
					// x,y,z
					if (!value.HasMember("x") || !value.HasMember("y") || !value.HasMember("z")) {
						X_ERROR("Ents", "Invalid origin");
						return false;
					}

					trans.pos.x = value["x"].GetFloat();
					trans.pos.y = value["y"].GetFloat();
					trans.pos.z = value["z"].GetFloat();
				}
				break;
				case "angles"_fnv1a:
				{
					Anglesf angles;
					if (!value.HasMember("p") || !value.HasMember("y") || !value.HasMember("r")) {
						X_ERROR("Ents", "Invalid angles");
						return false;
					}

					// fuck you make me wet.
					angles.setPitch(value["p"].GetFloat());
					angles.setYaw(value["y"].GetFloat());
					angles.setRoll(value["r"].GetFloat());

					trans.quat = angles.toQuat();
				}
				break;

				case "name"_fnv1a:
				{
					auto& entName = reg_.assign<EntName>(ent);
					if (value.GetType() != core::json::Type::kStringType) {
						X_ERROR("Ents", "Invalid name");
						return false;
					}

					entName.name = core::string(value.GetString(), value.GetStringLength());
				}
				break;

				default:
					X_WARNING("Ent", "Unknown ent member: \"%s\"", name.GetString());
					break;
			}
		}


		return true;
	}


} // namespace entity

X_NAMESPACE_END