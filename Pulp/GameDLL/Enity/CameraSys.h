#pragma once

#include "EnityComponents.h"

X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
	struct ICVar;
)


X_NAMESPACE_BEGIN(game)

namespace entity
{

	class CameraSystem
	{
	public:
		CameraSystem();
		~CameraSystem();

		bool init(void);
		void update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene);

		void setActiveEnt(EntityId entId);

	private:
		void OnCamPosChanged(core::ICVar* pVar);
		void OnCamAngChanged(core::ICVar* pVar);

	private:
		EntityId activeEnt_;

		bool setCamPos_;
		bool setCamAngle_;

		Vec3f cameraPos_;
		Vec3f cameraAngle_;

		XCamera cam_;
	};



} // namespace entity

X_NAMESPACE_END