#pragma once

#include <IPhysics.h>

X_NAMESPACE_BEGIN(game)


struct Position
{
	Vec3f pos;
};

struct Velocity 
{
	Vec3f dir;
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
	physics::ICapsuleCharacterController* pController;
};





X_NAMESPACE_END