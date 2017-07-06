#pragma once


#include "EnityComponents.h"

X_NAMESPACE_DECLARE(core,
	struct FrameTimeData;
	struct ICVar;
)


X_NAMESPACE_BEGIN(game)

namespace entity
{

	class PhysicsSystem
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		bool init(void);
		void update(core::FrameData& frame, EnitiyRegister& reg, physics::IScene* pPhysScene);



	private:
	};



} // namespace entity

X_NAMESPACE_END