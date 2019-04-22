#include "stdafx.h"


class TelemFixture : public benchmark::Fixture
{
public:
    void SetUp(const::benchmark::State& state)
    {
        const size_t telemBufSize = 1024 * 1024 * 16;
        static uint8_t telemBuf[telemBufSize];

        if (state.thread_index == 0) {
            X_ASSERT(ttLoadLibary(), "Failed to resolve telem")();
            X_ASSERT(ttInit(), "Failed to init telem")();

            ttInitializeContext(ctx, telemBuf, sizeof(telemBuf));
            ttTick(ctx);
        }
    }

    void TearDown(const ::benchmark::State& state)
    {
        if (state.thread_index == 0) {
            ttClose(ctx);
            ttShutdownContext(ctx);
            ttShutDown();
        }
    }

protected:
    telem::ContexHandle ctx;
};

BENCHMARK_DEFINE_F(TelemFixture, zone_paused)(benchmark::State& state) {
    ttPause(ctx, true);

    for (auto _ : state) {
        ttEnter(ctx, "meow");
        ttLeave(ctx);
    }
}

BENCHMARK_DEFINE_F(TelemFixture, zone_simple)(benchmark::State& state) {
    for (auto _ : state) {
        ttEnter(ctx, "meow");
        ttLeave(ctx);
    }
}


BENCHMARK_DEFINE_F(TelemFixture, zone_printf_str)(benchmark::State& state) {
    for (auto _ : state) {
        ttEnter(ctx, "this is a string %s in the middle", "with format spec");
        ttLeave(ctx);
    }
}


BENCHMARK_DEFINE_F(TelemFixture, zone_printf_int)(benchmark::State& state) {
    for (auto _ : state) {
        ttEnter(ctx, "this is a string %" PRIi32 ", %" PRIi64 " in the middle", 25251_i32, 123456_i64);
        ttLeave(ctx);
    }
}

BENCHMARK_DEFINE_F(TelemFixture, callstack_get)(benchmark::State& state) {
    TtCallStack stack;
    for (auto _ : state) {
        ttGetCallStack(ctx, stack);
    }
}

BENCHMARK_DEFINE_F(TelemFixture, lock_try)(benchmark::State& state) {
    int bogusHandle;
    for (auto _ : state) {
        ttTryLock(ctx, &bogusHandle, "AcquireSlot");
        ttEndTryLock(ctx, &bogusHandle, TtLockResult::Acquired);
    }
}

BENCHMARK_DEFINE_F(TelemFixture, lock_setstate)(benchmark::State& state) {
    int bogusHandle;
    for (auto _ : state) {
        ttSetLockState(ctx, &bogusHandle, TtLockState::Locked);
    }
}

BENCHMARK_DEFINE_F(TelemFixture, plot)(benchmark::State& state) {  
    for (auto _ : state) {
        ttPlotI32(ctx, TtPlotType::Integer, 1249, "my plot");
    }
}
BENCHMARK_REGISTER_F(TelemFixture, zone_paused);
BENCHMARK_REGISTER_F(TelemFixture, zone_simple);
BENCHMARK_REGISTER_F(TelemFixture, zone_simple)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_str);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_int);
BENCHMARK_REGISTER_F(TelemFixture, callstack_get);
BENCHMARK_REGISTER_F(TelemFixture, lock_try);
BENCHMARK_REGISTER_F(TelemFixture, lock_setstate);
BENCHMARK_REGISTER_F(TelemFixture, plot);

