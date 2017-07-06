#pragma once


#include <IInput.h>

#include "EnityComponents.h"

#include "CameraSys.h"
#include "InputSys.h"

X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
)

X_NAMESPACE_BEGIN(game)

namespace entity
{

	class EnititySystem 
	{
	public:
		typedef EnitiyRegister::entity_type EntityId;
		

	public:
		EnititySystem(core::MemoryArenaBase* arena);

		bool init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene);
		void update(core::FrameData& frame);

		EntityId createPlayer(const Vec3f& origin);
		EntityId createCamera(const Vec3f& origin);

		bool loadEntites(const char* pJsonBegin, const char* pJsonEnd);

	private:
		bool parseMiscModels(core::json::Value::Array val);
		bool parseScriptOrigins(core::json::Value::Array val);


	private:
		EnitiyRegister ecs_;

		physics::IPhysics* pPhysics_;
		physics::IScene* pPhysScene_;


		InputSystem inputSys_;
		CameraSystem cameraSys_;

	};


} // namespace entity


X_NAMESPACE_END