#pragma once

#include <IPhysics.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{


struct Velocity 
{
	Vec3f dir;
};

struct TransForm : public Transformf
{
	
};

struct Health
{
	int32_t hp;
};

struct PhysicsComponent
{
	physics::ActorHandle actor;
};

struct PhysicsTrigger
{
	physics::ActorHandle actor;
};

struct CharacterController
{
	physics::ICharacterController* pController;
};


struct ScriptName
{
	const char* pName;
};

struct Player
{
	Vec3f eyeOffset;
	Vec3f viewAngles;

	Anglesf deltaViewAngles;

	Vec3f cameraOrigin;
	Anglesf cameraAxis;
};


using EnitiyRegister = ecs::StandardRegistry<uint16_t,
	Velocity,
	TransForm,
	Health,
	PhysicsComponent,
	PhysicsTrigger,
	CharacterController,
	ScriptName,
	Player
>;

typedef EnitiyRegister::entity_type EntityId;


} // namespace entity

X_NAMESPACE_END