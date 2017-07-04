#pragma once

#include <IPhysics.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{


struct Position
{
	Vec3f pos;
};

struct Velocity 
{
	Vec3f dir;
};

struct TransForm
{
	Transformf trans;
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


} // namespace entity

X_NAMESPACE_END