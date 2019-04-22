#include "stdafx.h"


class TelemFixture : public benchmark::Fixture
{
public:
    void SetUp(const ::benchmark::State&)
    {
        const size_t telemBufSize = 1024 * 1024 * 2;
        static uint8_t telemBuf[telemBufSize];

        X_ASSERT(ttLoadLibary(), "Failed to resolve telem")();
        X_ASSERT(ttInit(), "Failed to init telem")();

        ttInitializeContext(ctx, telemBuf, sizeof(telemBuf));
        ttTick(ctx);
    }

    void TearDown(const ::benchmark::State&)
    {
        ttClose(ctx);
        ttShutdownContext(ctx);
        ttShutDown();
    }

protected:
    telem::ContexHandle ctx;
};

BENCHMARK_F(TelemFixture, zone_paused)(benchmark::State& state) {
    ttPause(ctx, true);

    for (auto _ : state) {
        ttEnter(ctx, "meow");
        ttLeave(ctx);
    }
}

BENCHMARK_F(TelemFixture, zone_simple)(benchmark::State& state) {
    for (auto _ : state) {
        ttEnter(ctx, "meow");
        ttLeave(ctx);
    }
}


BENCHMARK_F(TelemFixture, zone_printf_str)(benchmark::State& state) {
    for (auto _ : state) {
        ttEnter(ctx, "this is a string %s in the middle", "with format spec");
        ttLeave(ctx);
    }
}


BENCHMARK_F(TelemFixture, zone_printf_int)(benchmark::State& state) {
    for (auto _ : state) {
        ttEnter(ctx, "this is a string %" PRIi32 ", %" PRIi64 " in the middle", 25251_i32, 123456_i64);
        ttLeave(ctx);
    }
}

BENCHMARK_F(TelemFixture, callstack_get)(benchmark::State& state) {
    TtCallStack stack;
    for (auto _ : state) {
        ttGetCallStack(ctx, stack);
    }
}


BENCHMARK_REGISTER_F(TelemFixture, zone_paused);
BENCHMARK_REGISTER_F(TelemFixture, zone_simple);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_str);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_int);
BENCHMARK_REGISTER_F(TelemFixture, callstack_get);

