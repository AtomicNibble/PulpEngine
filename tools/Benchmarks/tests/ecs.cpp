#include "stdafx.h"

#include <../GameDLL/ECS/ComponentPool.h>
#include <../GameDLL/ECS/Registry.h>

namespace
{
    struct Position
    {
        uint64_t x;
        uint64_t y;
    };

    struct Velocity
    {
        uint64_t x;
        uint64_t y;
    };

    struct Comp1
    {
    };
    struct Comp2
    {
    };
    struct Comp3
    {
    };
    struct Comp4
    {
    };
    struct Comp5
    {
    };
    struct Comp6
    {
    };
    struct Comp7
    {
    };
    struct Comp8
    {
    };

} // namespace


void BM_ecs_construct(benchmark::State& state)
{
    using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

    registry_type registry(g_arena);

    const size_t size = state.range(0);

    registry.entIdReserve(size);

    while (state.KeepRunning()) {
        for (size_t i = 0; i < size; i++) {
            benchmark::DoNotOptimize(registry.create());
        }
    }
}

void BM_ecs_destroy(benchmark::State& state)
{
    using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

    registry_type registry(g_arena);
    registry_type::EntityArr entities(g_arena);

    const size_t size = state.range(0);

    registry.entIdReserve(size);
    registry.freelistReserve(size);
    entities.reserve(size);

    while (state.KeepRunning()) {

        state.PauseTiming();

        entities.clear();
        for (size_t i = 0; i < size; i++) {
            entities.push_back(registry.create());
        }

        state.ResumeTiming();

        for (auto entity : entities) {
            registry.destroy(entity);
        }
    }
}

void BM_ecs_iterate_single_component(benchmark::State& state)
{
    using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

    registry_type registry(g_arena);
    registry_type::EntityArr entities(g_arena);

    const size_t size = state.range(0);

    registry.entIdReserve(size);
    registry.compReserve<Position>(size);

    for (size_t i = 0; i < size; i++) {
        registry.create<Position>();
    }

    while (state.KeepRunning()) {

        auto view = registry.view<Position>();

        for (auto entity : view) {
            benchmark::DoNotOptimize(registry.get<Position>(entity));
        }
    }
}

void BM_ecs_iterate_two_component(benchmark::State& state)
{
    using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity>;

    registry_type registry(g_arena);
    registry_type::EntityArr entities(g_arena);

    const size_t size = state.range(0);

    registry.entIdReserve(size);
    registry.compReserve<Position>(size);
    registry.compReserve<Velocity>(size);

    for (size_t i = 0; i < size; i++) {
        registry.create<Position, Velocity>();
    }

    while (state.KeepRunning()) {

        auto view = registry.view<Position, Velocity>();

        for (auto entity : view) {
            benchmark::DoNotOptimize(registry.get<Position>(entity));
            benchmark::DoNotOptimize(registry.get<Velocity>(entity));
        }
    }
}

void BM_ecs_iterate_five_component(benchmark::State& state)
{
    using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity, Comp1, Comp2, Comp3>;

    registry_type registry(g_arena);
    registry_type::EntityArr entities(g_arena);

    const size_t size = state.range(0);

    registry.entIdReserve(size);
    registry.compReserve(size);

    for (size_t i = 0; i < size; i++) {
        registry.create<Position, Velocity, Comp1, Comp2, Comp3>();
    }

    while (state.KeepRunning()) {

        auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3>();

        for (auto entity : view) {
            benchmark::DoNotOptimize(registry.get<Position>(entity));
            benchmark::DoNotOptimize(registry.get<Velocity>(entity));
            benchmark::DoNotOptimize(registry.get<Comp1>(entity));
            benchmark::DoNotOptimize(registry.get<Comp2>(entity));
            benchmark::DoNotOptimize(registry.get<Comp3>(entity));
        }
    }
}

void BM_ecs_iterate_five_component_mixed(benchmark::State& state)
{
    using registry_type = game::ecs::StandardRegistry<uint32_t, Position, Velocity, Comp1, Comp2, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>;

    registry_type registry(g_arena);
    registry_type::EntityArr entities(g_arena);

    const size_t size = state.range(0);

    registry.entIdReserve(size);
    registry.compReserve(size);

    for (size_t i = 0; i < size; i++) {
        // don't have every component have required.
        // TODO: make this based on state.range(1)
        //       maybe use a fix seed PRNG
        if ((i % 2) == 0) {
            registry.create<Position, Comp1, Comp3, Comp4, Comp5, Comp6, Comp7, Comp8>();
        }
        else {
            registry.create<Position, Velocity, Comp1, Comp2, Comp3>();
        }
    }

    while (state.KeepRunning()) {

        auto view = registry.view<Position, Velocity, Comp1, Comp2, Comp3>();

        for (auto entity : view) {
            benchmark::DoNotOptimize(registry.get<Position>(entity));
            benchmark::DoNotOptimize(registry.get<Velocity>(entity));
            benchmark::DoNotOptimize(registry.get<Comp1>(entity));
            benchmark::DoNotOptimize(registry.get<Comp2>(entity));
            benchmark::DoNotOptimize(registry.get<Comp3>(entity));
        }
    }
}

BENCHMARK(BM_ecs_construct)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_ecs_destroy)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_ecs_iterate_single_component)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_ecs_iterate_two_component)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_ecs_iterate_five_component)->RangeMultiplier(2)->Range(16, 1 << 16);
BENCHMARK(BM_ecs_iterate_five_component_mixed)->RangeMultiplier(2)->Range(16, 1 << 16);
