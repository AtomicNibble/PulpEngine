#include "stdafx.h"

#include <../GameDLL/ECS/ComponentPool.h>
#include <../GameDLL/ECS/Registry.h>

X_USING_NAMESPACE;

TEST(ECSRegistry, Functionalities)
{
    using registry_type = game::ecs::StandardRegistry<uint32_t, int, char>;

    registry_type registry(g_arena);

    ASSERT_EQ(registry.size(), registry_type::size_type{0});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{0});
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
    ASSERT_TRUE((registry.hasAll<int, char>(e2)));
    ASSERT_FALSE((registry.hasAll<int, char>(e1)));

    ASSERT_EQ(registry.assign<int>(e1, 42), 42);
    ASSERT_EQ(registry.assign<char>(e1, 'c'), 'c');
    registry.remove<int>(e2);
    registry.remove<char>(e2);

    ASSERT_TRUE(registry.has<int>(e1));
    ASSERT_FALSE(registry.has<int>(e2));
    ASSERT_TRUE(registry.has<char>(e1));
    ASSERT_FALSE(registry.has<char>(e2));
    ASSERT_TRUE((registry.hasAll<int, char>(e1)));
    ASSERT_FALSE((registry.hasAll<int, char>(e2)));

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
    ASSERT_EQ(static_cast<const registry_type&>(registry).get<int>(e1), 1);
    ASSERT_EQ(static_cast<const registry_type&>(registry).get<int>(e2), 1);

    ASSERT_EQ(registry.size(), registry_type::size_type{3});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{3});
    ASSERT_FALSE(registry.empty());

    registry.destroy(e3);

    ASSERT_TRUE(registry.isValid(e1));
    ASSERT_TRUE(registry.isValid(e2));
    ASSERT_FALSE(registry.isValid(e3));

    ASSERT_EQ(registry.size(), registry_type::size_type{2});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{3});
    ASSERT_FALSE(registry.empty());

    registry.reset();

    ASSERT_EQ(registry.size(), registry_type::size_type{0});
    ASSERT_EQ(registry.capacity(), registry_type::size_type{0});
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
    ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{1});

    registry.assign<char>(e1);

    ASSERT_EQ(view.size(), typename registry_type::view_type<char>::size_type{2});

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

    ASSERT_EQ(view.size(), registry_type::size_type{0});

    registry.reset();
}

TEST(ECSRegistry, EmptyViewMultipleComponent)
{
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

    pool_type pool{g_arena, 0};

    ASSERT_TRUE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{0});
    ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{0});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{0});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_EQ(pool.entities<int>(), pool.entities<int>() + pool.size<int>());
    ASSERT_EQ(pool.entities<double>(), pool.entities<double>() + pool.size<double>());
    ASSERT_FALSE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
}

TEST(ECSComponentPool, ConstructDestroy)
{
    using pool_type = game::ecs::ComponentPool<uint8_t, double, int>;

    pool_type pool{g_arena, 4};

    ASSERT_EQ(pool.construct<int>(0, 42), 42);
    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_GE(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_GE(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{1});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_TRUE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_FALSE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));

    ASSERT_EQ(pool.construct<int>(1, 0), 0);
    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_GE(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_GE(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{2});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_TRUE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_TRUE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));
    ASSERT_NE(pool.get<int>(0), pool.get<int>(1));
    ASSERT_NE(&pool.get<int>(0), &pool.get<int>(1));

    pool.destroy<int>(0);
    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_GE(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_GE(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{1});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_FALSE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_TRUE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));

    pool.destroy<int>(1);
    ASSERT_TRUE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_GE(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_GE(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{0});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{0});
    ASSERT_FALSE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_FALSE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));

    int* comp[] = {
        &pool.construct<int>(0, 0),
        &pool.construct<int>(1, 1),
        nullptr,
        &pool.construct<int>(3, 3)};

    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_GE(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_GE(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{3});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
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
    const pool_type& cpool = pool;

    int& comp = pool.construct<int>(0, 42);

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

    pool_type pool{g_arena, 2};

    ASSERT_EQ(pool.construct<int>(0, 0), 0);
    ASSERT_EQ(pool.construct<int>(2, 2), 2);
    ASSERT_EQ(pool.construct<int>(3, 3), 3);
    ASSERT_EQ(pool.construct<int>(1, 1), 1);

    ASSERT_EQ(pool.size<int>(), decltype(pool.size<int>()){4});
    ASSERT_EQ(pool.entities<int>()[0], typename pool_type::entity_type{0});
    ASSERT_EQ(pool.entities<int>()[1], typename pool_type::entity_type{2});
    ASSERT_EQ(pool.entities<int>()[2], typename pool_type::entity_type{3});
    ASSERT_EQ(pool.entities<int>()[3], typename pool_type::entity_type{1});

    pool.destroy<int>(2);

    ASSERT_EQ(pool.size<int>(), decltype(pool.size<int>()){3});
    ASSERT_EQ(pool.entities<int>()[0], typename pool_type::entity_type{0});
    ASSERT_EQ(pool.entities<int>()[1], typename pool_type::entity_type{1});
    ASSERT_EQ(pool.entities<int>()[2], typename pool_type::entity_type{3});

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
