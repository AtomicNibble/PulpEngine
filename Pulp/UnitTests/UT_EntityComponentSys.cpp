#include "stdafx.h"

#include <Time\StopWatch.h>
#include <String\HumanDuration.h>

#include <../GameDLL/ECS/ComponentPool.h>
#include <../GameDLL/ECS/Registry.h>

X_USING_NAMESPACE;

namespace
{

	struct Position {
		uint64_t x;
		uint64_t y;
	};

	struct Velocity {
		uint64_t x;
		uint64_t y;
	};

	struct Comp1 {};
	struct Comp2 {};
	struct Comp3 {};
	struct Comp4 {};
	struct Comp5 {};
	struct Comp6 {};
	struct Comp7 {};
	struct Comp8 {};

} // namespace


TEST(ECSRegistry, Functionalities)
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, int, char>;

	registry_type registry(g_arena);

	ASSERT_EQ(registry.size(), registry_type::size_type{ 0 });
	ASSERT_EQ(registry.capacity(), registry_type::size_type{ 0 });
	ASSERT_TRUE(registry.empty());

	ASSERT_TRUE(registry.empty<int>());
	ASSERT_TRUE(registry.empty<char>());

	registry_type::entity_type e1 = registry.create();
	registry_type::entity_type e2 = registry.create<int, char>();

	ASSERT_FALSE(registry.empty<int>());
	ASSERT_FALSE(registry.empty<char>());

	ASSERT_NE(e1, e2);

	ASSERT_FALSE(registry.has<int>(e1));
	ASSERT_TRUE(registry.has<int>(e2));
	ASSERT_FALSE(registry.has<char>(e1));
	ASSERT_TRUE(registry.has<char>(e2));
	ASSERT_TRUE((registry.has<int, char>(e2)));
	ASSERT_FALSE((registry.has<int, char>(e1)));

	ASSERT_EQ(registry.assign<int>(e1, 42), 42);
	ASSERT_EQ(registry.assign<char>(e1, 'c'), 'c');
	registry.remove<int>(e2);
	registry.remove<char>(e2);

	ASSERT_TRUE(registry.has<int>(e1));
	ASSERT_FALSE(registry.has<int>(e2));
	ASSERT_TRUE(registry.has<char>(e1));
	ASSERT_FALSE(registry.has<char>(e2));
	ASSERT_TRUE((registry.has<int, char>(e1)));
	ASSERT_FALSE((registry.has<int, char>(e2)));

	registry_type::entity_type e3 = registry.clone(e1);

	ASSERT_TRUE(registry.has<int>(e3));
	ASSERT_TRUE(registry.has<char>(e3));
	ASSERT_EQ(registry.get<int>(e1), 42);
	ASSERT_EQ(registry.get<char>(e1), 'c');
	ASSERT_EQ(registry.get<int>(e1), registry.get<int>(e3));
	ASSERT_EQ(registry.get<char>(e1), registry.get<char>(e3));
	ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e3));
	ASSERT_NE(&registry.get<char>(e1), &registry.get<char>(e3));

	registry.copy(e2, e1);
	ASSERT_TRUE(registry.has<int>(e2));
	ASSERT_TRUE(registry.has<char>(e2));
	ASSERT_EQ(registry.get<int>(e1), 42);
	ASSERT_EQ(registry.get<char>(e1), 'c');
	ASSERT_EQ(registry.get<int>(e1), registry.get<int>(e2));
	ASSERT_EQ(registry.get<char>(e1), registry.get<char>(e2));
	ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e2));
	ASSERT_NE(&registry.get<char>(e1), &registry.get<char>(e2));

	registry.replace<int>(e1, 0);
	ASSERT_EQ(registry.get<int>(e1), 0);
	registry.copy<int>(e2, e1);
	ASSERT_EQ(registry.get<int>(e2), 0);
	ASSERT_NE(&registry.get<int>(e1), &registry.get<int>(e2));

	registry.remove<int>(e2);
	registry.accomodate<int>(e1, 1);
	registry.accomodate<int>(e2, 1);
	ASSERT_EQ(static_cast<const registry_type &>(registry).get<int>(e1), 1);
	ASSERT_EQ(static_cast<const registry_type &>(registry).get<int>(e2), 1);

	ASSERT_EQ(registry.size(), registry_type::size_type{ 3 });
	ASSERT_EQ(registry.capacity(), registry_type::size_type{ 3 });
	ASSERT_FALSE(registry.empty());

	registry.destroy(e3);

	ASSERT_TRUE(registry.isValid(e1));
	ASSERT_TRUE(registry.isValid(e2));
	ASSERT_FALSE(registry.isValid(e3));

	ASSERT_EQ(registry.size(), registry_type::size_type{ 2 });
	ASSERT_EQ(registry.capacity(), registry_type::size_type{ 3 });
	ASSERT_FALSE(registry.empty());

	registry.reset();

	ASSERT_EQ(registry.size(), registry_type::size_type{ 0 });
	ASSERT_EQ(registry.capacity(), registry_type::size_type{ 0 });
	ASSERT_TRUE(registry.empty());

	registry.create<int, char>();

	ASSERT_FALSE(registry.empty<int>());
	ASSERT_FALSE(registry.empty<char>());

	registry.reset<int>();

	ASSERT_TRUE(registry.empty<int>());
	ASSERT_FALSE(registry.empty<char>());

	registry.reset();

	ASSERT_TRUE(registry.empty<int>());
	ASSERT_TRUE(registry.empty<char>());

	e1 = registry.create<int>();
	e2 = registry.create();

	registry.reset<int>(e1);
	registry.reset<int>(e2);
	ASSERT_TRUE(registry.empty<int>());
}

TEST(ECSRegistry, Copy) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, int, char, double>;

	registry_type registry(g_arena);

	registry_type::entity_type e1 = registry.create<int, char>();
	registry_type::entity_type e2 = registry.create<int, double>();

	ASSERT_TRUE(registry.has<int>(e1));
	ASSERT_TRUE(registry.has<char>(e1));
	ASSERT_FALSE(registry.has<double>(e1));

	ASSERT_TRUE(registry.has<int>(e2));
	ASSERT_FALSE(registry.has<char>(e2));
	ASSERT_TRUE(registry.has<double>(e2));

	registry.copy(e2, e1);

	ASSERT_TRUE(registry.has<int>(e1));
	ASSERT_TRUE(registry.has<char>(e1));
	ASSERT_FALSE(registry.has<double>(e1));

	ASSERT_TRUE(registry.has<int>(e2));
	ASSERT_TRUE(registry.has<char>(e2));
	ASSERT_FALSE(registry.has<double>(e2));

	ASSERT_FALSE(registry.empty<int>());
	ASSERT_FALSE(registry.empty<char>());
	ASSERT_TRUE(registry.empty<double>());

	registry.reset();
}

TEST(ECSRegistry, ViewSingleComponent)
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, int, char>;

	registry_type registry(g_arena);

	registry_type::entity_type e1 = registry.create();
	registry_type::entity_type e2 = registry.create<int, char>();

	auto view = registry.view<char>();

	ASSERT_NE(view.begin(), view.end());
	ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{ 1 });

	registry.assign<char>(e1);

	ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{ 2 });

	registry.remove<char>(e1);
	registry.remove<char>(e2);

	ASSERT_EQ(view.begin(), view.end());
	registry.reset();

	registry.view<char>().begin()++;
	++registry.view<char>().begin();
}

TEST(ECSRegistry, ViewMultipleComponent)
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, int, char>;

	registry_type registry(g_arena);

	registry_type::entity_type e1 = registry.create<char>();
	registry_type::entity_type e2 = registry.create<int, char>();

	auto view = registry.view<int, char>();

	ASSERT_NE(view.begin(), view.end());

	registry.remove<char>(e1);
	registry.remove<char>(e2);
	view.reset();

	ASSERT_EQ(view.begin(), view.end());
	registry.reset();

	(registry.view<int, char>().begin()++);
	(++registry.view<int, char>().begin());
}

TEST(ECSRegistry, EmptyViewSingleComponent) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, char, int, double>;

	registry_type registry(g_arena);

	registry.create<char, double>();
	registry.create<char>();

	auto view = registry.view<int>();

	ASSERT_EQ(view.size(), registry_type::size_type{ 0 });

	registry.reset();
}

TEST(ECSRegistry, EmptyViewMultipleComponent) {
	using registry_type = game::ecs::StandardRegistry<uint32_t, char, int, float, double>;

	registry_type registry(g_arena);

	registry.create<double, int, float>();
	registry.create<char, float>();

	auto view = registry.view<char, int, float>();

	for (auto entity : view) {
		(void)entity;
		FAIL();
	}

	registry.reset();
}

TEST(ECSRegistry, ViewSingleComponentWithExclude) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, int, char>;

	registry_type registry(g_arena);

	registry_type::entity_type e1 = registry.create<char>();
	registry_type::entity_type e2 = registry.create<int, char>();

	auto view = registry.view<char>().exclude<int>();

	ASSERT_NE(view.begin(), view.end());

	ASSERT_EQ(*view.begin(), e1);
	ASSERT_NE(*view.begin(), e2);
	ASSERT_EQ(++view.begin(), view.end());
	registry.reset();

	(registry.view<char>().exclude<int>().begin()++);
	(++registry.view<char>().exclude<int>().begin());
}

TEST(ECSRegistry, ViewMultipleComponentWithExclude) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, int, char, double>;

	registry_type registry(g_arena);

	registry_type::entity_type e1 = registry.create<int, char, double>();
	registry_type::entity_type e2 = registry.create<char, double>();

	auto view = registry.view<char, double>().exclude<int>();

	ASSERT_NE(view.begin(), view.end());

	ASSERT_NE(*view.begin(), e1);
	ASSERT_EQ(*view.begin(), e2);
	ASSERT_EQ(++view.begin(), view.end());
	registry.reset();

	(registry.view<char>().exclude<int>().begin()++);
	(++registry.view<char>().exclude<int>().begin());
}

// -----------------------------------------------------------------------

TEST(ECSComponentPool, Functionalities) 
{
	using pool_type = game::ecs::ComponentPool<uint8_t, int, double>;

	pool_type pool{ g_arena, 0 };

	ASSERT_TRUE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<double>());
	ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{ 0 });
	ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{ 0 });
	ASSERT_EQ(pool.size<int>(), pool_type::size_type{ 0 });
	ASSERT_EQ(pool.size<double>(), pool_type::size_type{ 0 });
	ASSERT_EQ(pool.entities<int>(), pool.entities<int>() + pool.size<int>());
	ASSERT_EQ(pool.entities<double>(), pool.entities<double>() + pool.size<double>());
	ASSERT_FALSE(pool.has<int>(0));
	ASSERT_FALSE(pool.has<double>(0));
}

TEST(ECSComponentPool, ConstructDestroy) 
{
	using pool_type = game::ecs::ComponentPool<uint8_t, double, int>;

	pool_type pool{ g_arena, 4 };


	ASSERT_EQ(pool.construct<int>(0, 42), 42);
	ASSERT_FALSE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<double>());
	ASSERT_GE(pool.capacity<int>(), pool_type::size_type{ 4 });
	ASSERT_GE(pool.capacity<double>(), pool_type::size_type{ 4 });
	ASSERT_EQ(pool.size<int>(), pool_type::size_type{ 1 });
	ASSERT_EQ(pool.size<double>(), pool_type::size_type{ 0 });
	ASSERT_TRUE(pool.has<int>(0));
	ASSERT_FALSE(pool.has<double>(0));
	ASSERT_FALSE(pool.has<int>(1));
	ASSERT_FALSE(pool.has<double>(1));

	ASSERT_EQ(pool.construct<int>(1), 0);
	ASSERT_FALSE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<double>());
	ASSERT_GE(pool.capacity<int>(), pool_type::size_type{ 4 });
	ASSERT_GE(pool.capacity<double>(), pool_type::size_type{ 4 });
	ASSERT_EQ(pool.size<int>(), pool_type::size_type{ 2 });
	ASSERT_EQ(pool.size<double>(), pool_type::size_type{ 0 });
	ASSERT_TRUE(pool.has<int>(0));
	ASSERT_FALSE(pool.has<double>(0));
	ASSERT_TRUE(pool.has<int>(1));
	ASSERT_FALSE(pool.has<double>(1));
	ASSERT_NE(pool.get<int>(0), pool.get<int>(1));
	ASSERT_NE(&pool.get<int>(0), &pool.get<int>(1));

	pool.destroy<int>(0);
	ASSERT_FALSE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<double>());
	ASSERT_GE(pool.capacity<int>(), pool_type::size_type{ 4 });
	ASSERT_GE(pool.capacity<double>(), pool_type::size_type{ 4 });
	ASSERT_EQ(pool.size<int>(), pool_type::size_type{ 1 });
	ASSERT_EQ(pool.size<double>(), pool_type::size_type{ 0 });
	ASSERT_FALSE(pool.has<int>(0));
	ASSERT_FALSE(pool.has<double>(0));
	ASSERT_TRUE(pool.has<int>(1));
	ASSERT_FALSE(pool.has<double>(1));

	pool.destroy<int>(1);
	ASSERT_TRUE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<double>());
	ASSERT_GE(pool.capacity<int>(), pool_type::size_type{ 4 });
	ASSERT_GE(pool.capacity<double>(), pool_type::size_type{ 4 });
	ASSERT_EQ(pool.size<int>(), pool_type::size_type{ 0 });
	ASSERT_EQ(pool.size<int>(), pool_type::size_type{ 0 });
	ASSERT_FALSE(pool.has<int>(0));
	ASSERT_FALSE(pool.has<double>(0));
	ASSERT_FALSE(pool.has<int>(1));
	ASSERT_FALSE(pool.has<double>(1));

	int *comp[] = {
		&pool.construct<int>(0, 0),
		&pool.construct<int>(1, 1),
		nullptr,
		&pool.construct<int>(3, 3)
	};

	ASSERT_FALSE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<double>());
	ASSERT_GE(pool.capacity<int>(), pool_type::size_type{ 4 });
	ASSERT_GE(pool.capacity<double>(), pool_type::size_type{ 4 });
	ASSERT_EQ(pool.size<int>(), pool_type::size_type{ 3 });
	ASSERT_EQ(pool.size<double>(), pool_type::size_type{ 0 });
	ASSERT_TRUE(pool.has<int>(0));
	ASSERT_FALSE(pool.has<double>(0));
	ASSERT_TRUE(pool.has<int>(1));
	ASSERT_FALSE(pool.has<double>(1));
	ASSERT_FALSE(pool.has<int>(2));
	ASSERT_FALSE(pool.has<double>(2));
	ASSERT_TRUE(pool.has<int>(3));
	ASSERT_FALSE(pool.has<double>(3));
	ASSERT_EQ(&pool.get<int>(0), comp[0]);
	ASSERT_EQ(&pool.get<int>(1), comp[1]);
	ASSERT_EQ(&pool.get<int>(3), comp[3]);
	ASSERT_EQ(pool.get<int>(0), 0);
	ASSERT_EQ(pool.get<int>(1), 1);
	ASSERT_EQ(pool.get<int>(3), 3);

	pool.destroy<int>(0);
	pool.destroy<int>(1);
	pool.destroy<int>(3);
}

TEST(ECSComponentPool, HasGet) 
{
	using pool_type = game::ecs::ComponentPool<uint8_t, int, char>;

	pool_type pool(g_arena);
	const pool_type &cpool = pool;

	int &comp = pool.construct<int>(0, 42);

	ASSERT_EQ(pool.get<int>(0), comp);
	ASSERT_EQ(pool.get<int>(0), 42);
	ASSERT_TRUE(pool.has<int>(0));

	ASSERT_EQ(cpool.get<int>(0), comp);
	ASSERT_EQ(cpool.get<int>(0), 42);
	ASSERT_TRUE(cpool.has<int>(0));

	pool.destroy<int>(0);
}

TEST(ECSComponentPool, EntitiesReset) 
{
	using pool_type = game::ecs::ComponentPool<uint8_t, int, char>;

	pool_type pool{ g_arena, 2 };

	ASSERT_EQ(pool.construct<int>(0, 0), 0);
	ASSERT_EQ(pool.construct<int>(2, 2), 2);
	ASSERT_EQ(pool.construct<int>(3, 3), 3);
	ASSERT_EQ(pool.construct<int>(1, 1), 1);

	ASSERT_EQ(pool.size<int>(), decltype(pool.size<int>()){4});
	ASSERT_EQ(pool.entities<int>()[0], typename pool_type::entity_type{ 0 });
	ASSERT_EQ(pool.entities<int>()[1], typename pool_type::entity_type{ 2 });
	ASSERT_EQ(pool.entities<int>()[2], typename pool_type::entity_type{ 3 });
	ASSERT_EQ(pool.entities<int>()[3], typename pool_type::entity_type{ 1 });

	pool.destroy<int>(2);

	ASSERT_EQ(pool.size<int>(), decltype(pool.size<int>()){3});
	ASSERT_EQ(pool.entities<int>()[0], typename pool_type::entity_type{ 0 });
	ASSERT_EQ(pool.entities<int>()[1], typename pool_type::entity_type{ 1 });
	ASSERT_EQ(pool.entities<int>()[2], typename pool_type::entity_type{ 3 });

	ASSERT_EQ(pool.construct<char>(0, 'c'), 'c');

	ASSERT_FALSE(pool.empty<int>());
	ASSERT_FALSE(pool.empty<char>());

	pool.reset<char>();

	ASSERT_FALSE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<char>());

	pool.reset();

	ASSERT_TRUE(pool.empty<int>());
	ASSERT_TRUE(pool.empty<char>());
}

// ------------------------------------------------

TEST(ECSRegistryBench, Construct) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

	registry_type registry(g_arena);

	X_LOG0("ECS", "Constructing 1000000 entities");

	registry.reserve(100'000);

	core::StopWatch timer;
	for (uint64_t i = 0; i < 100'000L; i++) {
		registry.create();
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}

TEST(ECSRegistryBench, Destroy) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

	registry_type registry(g_arena);
	registry_type::EntityArr entities(g_arena);

	registry.reserve(100'000);
	registry.availableReserve(100'000);
	entities.reserve(100'000);

	X_LOG0("ECS", "Destroying 100000 entities");

	for (uint64_t i = 0; i < 100'000L; i++) {
		entities.push_back(registry.create());
	}

	core::StopWatch timer;

	for (auto entity : entities) {
		registry.destroy(entity);
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
}

TEST(ECSRegistryBench, IterateCreateDeleteSingleComponent)
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

	registry_type registry(g_arena);
	registry.reserve(1'000);
	registry.availableReserve(1'000);

	X_LOG0("ECS", "Looping 1000 times creating and deleting a random number of entities");

	core::random::XorShift rand;
	
	core::StopWatch timer;

	for (int32_t i = 0; i < 1000; i++) 
	{
		for (int32_t j = 0; j < 1000; j++)
		{
			registry.create<Position>();
		}

		auto view = registry.view<Position>();

		for (auto entity : view)
		{
			if (rand.rand() % 2 == 0)
			{
				registry.destroy(entity);
			}
		}
	}

	X_LOG0("ECS", "Left: %" PRIuS, registry.size());

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}

TEST(ECSRegistryBench, IterateSingleComponent100K) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

	registry_type registry(g_arena);
	registry.reserve(100'000);

	X_LOG0("ECS", "Iterating over 100000 entities, one component");

	for (uint64_t i = 0; i < 100'000L; i++) {
		registry.create<Position>();
	}

	core::StopWatch timer;

	auto view = registry.view<Position>();

	for (auto entity : view) {
		auto& position = registry.get<Position>(entity);
		(void)position;
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}

TEST(ECSRegistryBench, IterateTwoComponents100K)
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

	registry_type registry(g_arena);
	registry.reserve(100'000);

	X_LOG0("ECS", "Iterating over 100000 entities, two components");

	for (uint64_t i = 0; i < 100'000L; i++) {
		registry.create<Position, Velocity>();
	}

	core::StopWatch timer;

	auto view = registry.view<Position, Velocity>();

	for (auto entity : view) {
		auto& position = registry.get<Position>(entity);
		auto& velocity = registry.get<Velocity>(entity);
		(void)position;
		(void)velocity;
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}


TEST(ECSRegistryBench, IterateSingleComponent500K) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

	registry_type registry(g_arena);
	registry.reserve(500'000);

	X_LOG0("ECS", "Iterating over 500000 entities, one component");

	for (uint64_t i = 0; i < 500,000L; i++) {
		registry.create<Position>();
	}

	core::StopWatch timer;

	auto view = registry.view<Position>();

	for (auto entity : view) {
		auto& position = registry.get<Position>(entity);
		(void)position;
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}

TEST(ECSRegistryBench, IterateTwoComponents500K) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

	registry_type registry(g_arena);
	registry.reserve(500'000);

	X_LOG0("ECS", "Iterating over 500000 entities, two components");

	for (uint64_t i = 0; i < 500'000L; i++) {
		registry.create<Position, Velocity>();
	}

	core::StopWatch timer;

	auto view = registry.view<Position, Velocity>();

	for (auto entity : view) {
		auto& position = registry.get<Position>(entity);
		auto& velocity = registry.get<Velocity>(entity);
		(void)position;
		(void)velocity;
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}

TEST(ECSRegistryBench, IterateFiveComponents100K)
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity, Comp1, Comp2, Comp3>;

	registry_type registry(g_arena);
	registry.reserve(100'000);

	X_LOG0("ECS", "Iterating over 100000 entities, five components");

	for (uint64_t i = 0; i < 100'000L; i++) {
		registry.create<Position, Velocity, Comp1, Comp2, Comp3>();
	}

	core::StopWatch timer;

	auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3>();

	for (auto entity : view) {
		auto& position = registry.get<Position>(entity);
		auto& velocity = registry.get<Velocity>(entity);
		auto& comp1 = registry.get<Comp1>(entity);
		auto& comp2 = registry.get<Comp2>(entity);
		auto& comp3 = registry.get<Comp3>(entity);
		(void)position;
		(void)velocity;
		(void)comp1;
		(void)comp2;
		(void)comp3;
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}

TEST(ECSRegistryBench, IterateTenComponents100K) 
{
	using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>;

	registry_type registry(g_arena);
	registry.reserve(100'000);

	X_LOG0("ECS", "Iterating over 100000 entities, ten components");

	for (uint64_t i = 0; i < 100'000L; i++) {
		registry.create<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();
	}

	core::StopWatch timer;

	auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();

	for (auto entity : view) {
		auto& position = registry.get<Position>(entity);
		auto& velocity = registry.get<Velocity>(entity);
		auto& comp1 = registry.get<Comp1>(entity);
		auto& comp2 = registry.get<Comp2>(entity);
		auto& comp3 = registry.get<Comp3>(entity);
		auto& comp4 = registry.get<Comp4>(entity);
		auto& comp5 = registry.get<Comp5>(entity);
		auto& comp6 = registry.get<Comp6>(entity);
		auto& comp7 = registry.get<Comp7>(entity);
		auto& comp8 = registry.get<Comp8>(entity);
		(void)position;
		(void)velocity;
		(void)comp1;
		(void)comp2;
		(void)comp3;
		(void)comp4; 
		(void)comp5;
		(void)comp6;
		(void)comp7;
		(void)comp8;
	}

	core::HumanDuration::Str durStr;
	X_LOG0("ECS", "elapsed: ^6%s", core::HumanDuration::toString(durStr, timer.GetMilliSeconds()));
	registry.reset();
}