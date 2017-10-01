#pragma once

#include <IPhysics.h>
#include <Time\TimeVal.h>

#include "UserCmds\UserCmd.h"

X_NAMESPACE_DECLARE(model,
	class XModel;
)

X_NAMESPACE_DECLARE(anim,
	class Animator;
)

X_NAMESPACE_DECLARE(engine,
	struct IRenderEnt;
)


X_NAMESPACE_BEGIN(game)

namespace entity
{

typedef uint16_t EntityId;

struct TransForm : public Transformf
{

};

struct Health
{
	int32_t hp;
	int32_t max;
};

struct SoundObject
{
	Vec3f offset; // offset of sound object relative to ent's transform.
	core::string occType;
	sound::SndObjectHandle handle;
};

struct SoundEnviroment
{
	// sound::SndObjectHandle handle;
};


struct Mesh
{
	core::string name;
	model::XModel* pModel;
};

struct MeshRenderer
{
	engine::IRenderEnt* pRenderEnt;
};

struct MeshCollider
{
	physics::ActorHandle actor;

};


struct Animator
{
	anim::Animator* pAnimator;

};


struct EntName
{
	core::string name;
};


// -----------------------------------

struct Inventory
{

	// i will want per ammo type stores at somepoint
	// but the types are data driven.
	// need some sort of type to index shit maybe.
	int32_t ammo; 


};

struct Velocity 
{
	Vec3f dir;
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

struct RenderComponent
{
	engine::IRenderEnt* pRenderEnt;
	model::XModel* pModel;
};


// struct ScriptName
// {
// 	const char* pName;
// };

struct RenderView
{
	Vec2f fov;

	Vec3f viewOrg;
	Matrix33f viewAxis;
};



struct Player
{
	X_DECLARE_FLAGS(State) (
		Jump,
		Crouch,
		Run,
		OnGround
	);

	typedef Flags<State> StateFlags;

	StateFlags state;

	core::TimeVal jumpTime;

	Vec3f eyeOffset;

	Anglesf viewAngles;
	Anglesf cmdAngles;
	Anglesf deltaViewAngles;

	Anglesf viewBobAngles;
	Vec3f	viewBob;

	float bobFrac;
	float bobfracsin;
	int32_t	bobCycle;

	Vec3f		firstPersonViewOrigin;
	Matrix33f	firstPersonViewAxis;

	UserCmd oldUserCmd;
	UserCmd userCmd;
};

struct Attached
{
	EntityId parentEnt;
	model::BoneHandle parentBone; // not always set

	Vec3f offset;

};

using EnitiyRegister = ecs::StandardRegistry<EntityId,
	TransForm,
	Health,

	Mesh,
	MeshRenderer,
	MeshCollider,

	SoundObject,
	SoundEnviroment,

	Inventory,
	Attached,

	Animator,
	Velocity,
	RenderComponent,
	PhysicsComponent,
	PhysicsTrigger,
	CharacterController,
	EntName,
	Player
>;

constexpr EnitiyRegister::entity_type INVALID_ENT_ID = EnitiyRegister::INVALID_ID;

} // namespace entity

X_NAMESPACE_END