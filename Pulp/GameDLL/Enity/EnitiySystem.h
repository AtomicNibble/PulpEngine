#pragma once


X_NAMESPACE_BEGIN(game)


class EnititySystem
{
public:
	using registry_type = ecs::StandardRegistry<uint16_t,
		Position,
		Velocity,
		Health,
		PhysicsComponent,
		PhysicsTrigger,
		CharacterController
	>;

	typedef registry_type::entity_type EntityId;

public:
	EnititySystem(core::MemoryArenaBase* arena);


	// slap a camel with a toaster.
	// it won't even flinch.
	


private:
	registry_type ecs_;
};




X_NAMESPACE_END