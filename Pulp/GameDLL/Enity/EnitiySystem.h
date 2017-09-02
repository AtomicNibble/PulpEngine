#pragma once


#include <IInput.h>
#include <IModelManager.h>

#include "EnityComponents.h"

#include "CameraSys.h"
#include "InputSys.h"
#include "PlayerSys.h"
#include "SoundSys.h"
#include "PhysicsSystem.h"

#include "DataTranslator.h"

X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
)

X_NAMESPACE_DECLARE(engine,
	struct IWorld3D;
)

X_NAMESPACE_BEGIN(game)

class GameVars;
class UserCmdMan;

namespace entity
{
	static const EnitiyRegister::entity_type INVALID_ID = EnitiyRegister::INVALID_ID;

	class EnititySystem 
	{
	public:
		typedef EnitiyRegister::entity_type EntityId;
		
	public:
		EnititySystem(GameVars& vars, core::MemoryArenaBase* arena);

		bool init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld);
		void update(core::FrameData& frame, UserCmdMan& userCmdMan);

		EntityId createEnt(void);

		void makePlayer(EntityId id);
		bool addController(EntityId id);

		bool loadEntites(const char* pJsonBegin, const char* pJsonEnd);
		bool loadEntites2(const char* pJsonBegin, const char* pJsonEnd);

		X_INLINE const EnitiyRegister& getRegister(void) const;
		X_INLINE EnitiyRegister& getRegister(void);

	private:
		bool createTranslatours(void);

		// Temp
		bool parseMiscModels(core::json::Value::Array val);
		bool parseScriptOrigins(core::json::Value::Array val);


		template<typename CompnentT>
		static bool parseComponent(DataTranslator<CompnentT>& translator, CompnentT& comp, const core::json::Value& compDesc);

		bool parseEntDesc(core::json::Value& val);

	private:
		core::MemoryArenaBase* arena_;
		EnitiyRegister reg_;
		GameVars& vars_;

		physics::IPhysics* pPhysics_;
		physics::IScene* pPhysScene_;
		engine::IWorld3D* p3DWorld_;
		model::IModelManager* pModelManager_;

		PlayerSystem playerSys_;
		CameraSystem cameraSys_;
		SoundSystem soundSys_;
		PhysicsSystem physSys_;

	private:
		DataTranslator<Health> dtHealth_;
		DataTranslator<Mesh> dtMesh_;
		DataTranslator<SoundObject> dtSoundObj_;
	};


	X_INLINE const EnitiyRegister& EnititySystem::getRegister(void) const 
	{
		return reg_;
	}

	X_INLINE EnitiyRegister& EnititySystem::getRegister(void) 
	{
		return reg_;
	}

} // namespace entity


X_NAMESPACE_END