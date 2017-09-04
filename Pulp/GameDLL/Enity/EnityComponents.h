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


struct EntName
{
	core::string name;
};


// -----------------------------------

struct Animated
{
	anim::Animator* pAnimator;

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


using EnitiyRegister = ecs::StandardRegistry<uint16_t,
	TransForm,
	Health,

	Mesh,
	MeshRenderer,
	MeshCollider,

	SoundObject,
	SoundEnviroment,

	Animated,
	Velocity,
	RenderComponent,
	PhysicsComponent,
	PhysicsTrigger,
	CharacterController,
	EntName,
	Player
>;

typedef EnitiyRegister::entity_type EntityId;


} // namespace entity

X_NAMESPACE_END