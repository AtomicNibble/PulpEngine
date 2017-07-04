#pragma once


#include <IInput.h>


X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
)

X_NAMESPACE_BEGIN(game)

namespace entity
{
	using EnitiyRegister = ecs::StandardRegistry<uint16_t,
		Position,
		Velocity,
		TransForm,
		Health,
		PhysicsComponent,
		PhysicsTrigger,
		CharacterController,
		ScriptName
	>;

	typedef EnitiyRegister::entity_type EntityId;



	class InputSystem :
		public input::IInputEventListner
	{
	public:

		bool init(void);
		void update(core::FrameTimeData& timeInfo, EnitiyRegister& reg, physics::IScene* pPhysScene);

	private:
		void processInput(core::FrameTimeData& timeInfo);

	private:

		// IInputEventListner
		bool OnInputEvent(const input::InputEvent& event) X_FINAL;
		bool OnInputEventChar(const input::InputEvent& event) X_FINAL;
		// ~IInputEventListner

	private:
		input::InputEventBuffer inputEvents_;
	};

	class CameraSystem 
	{
	public:
		CameraSystem();
		~CameraSystem();

		bool init(void);
		void update(core::FrameData& frame, EnitiyRegister& reg);

		void setActiveEnt(EntityId entId);


	private:
		EntityId activeEnt_;
		Vec3f cameraPos_;
		Vec3f cameraAngle_;

		XCamera cam_;
	};

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