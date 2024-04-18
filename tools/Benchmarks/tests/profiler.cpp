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

            ttInitializeContext(&ctx, telemBuf, sizeof(telemBuf));
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

BENCHMARK_DEFINE_F(TelemFixture, zone_simple_cpp)(benchmark::State& state) {
    for (auto _ : state) {
        ttZone(ctx, "meow");
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

BENCHMARK_DEFINE_F(TelemFixture, callstack_send)(benchmark::State& state) {
    TtCallStack stack;
    ttGetCallStack(ctx, stack);
    for (auto _ : state) {
        ttSendCallStack(ctx, &stack);
    }
}

BENCHMARK_DEFINE_F(TelemFixture, lock_try)(benchmark::State& state) {
    int bogusHandle;
    for (auto _ : state) {
        ttTryLock(ctx, &bogusHandle, "AcquireSlot");
        ttEndTryLock(ctx, &bogusHandle, TtLockResultAcquired);
    }

    X_UNUSED(bogusHandle);
}

BENCHMARK_DEFINE_F(TelemFixture, lock_setstate)(benchmark::State& state) {
    int bogusHandle;
    for (auto _ : state) {
        ttSetLockState(ctx, &bogusHandle, TtLockStateLocked);
    }

    X_UNUSED(bogusHandle);
}

BENCHMARK_DEFINE_F(TelemFixture, plot)(benchmark::State& state) {  
    for (auto _ : state) {
        ttPlotI32(ctx, TtPlotTypeInteger, 1249, "my plot");
    }
}

BENCHMARK_DEFINE_F(TelemFixture, alloc)(benchmark::State& state) {
    for (auto _ : state) {
        ttAlloc(ctx, &state, 1024, "my alloc");
    }
}

BENCHMARK_DEFINE_F(TelemFixture, free)(benchmark::State& state) {
    for (auto _ : state) {
        ttFree(ctx, &state);
    }
}

BENCHMARK_DEFINE_F(TelemFixture, msg)(benchmark::State& state) {
    for (auto _ : state) {
        ttMessage(ctx, TtMsgFlagsSeverityMsg, "this is a msg for you!");
    }
}

BENCHMARK_DEFINE_F(TelemFixture, msg_printf)(benchmark::State& state) {
    for (auto _ : state) {
        ttMessage(ctx, TtMsgFlagsSeverityMsg, "this is a string %" PRIi32 ", %" PRIi64 " in the middle", 25251_i32, 123456_i64);
    }
}

BENCHMARK_REGISTER_F(TelemFixture, zone_paused);
BENCHMARK_REGISTER_F(TelemFixture, zone_simple);
BENCHMARK_REGISTER_F(TelemFixture, zone_simple)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, zone_simple_cpp);
BENCHMARK_REGISTER_F(TelemFixture, zone_simple_cpp)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_str);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_str)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_int);
BENCHMARK_REGISTER_F(TelemFixture, zone_printf_int)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, callstack_get);
BENCHMARK_REGISTER_F(TelemFixture, callstack_send);
BENCHMARK_REGISTER_F(TelemFixture, lock_try);
BENCHMARK_REGISTER_F(TelemFixture, lock_try)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, lock_setstate);
BENCHMARK_REGISTER_F(TelemFixture, lock_setstate)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, plot);
BENCHMARK_REGISTER_F(TelemFixture, plot)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, alloc);
BENCHMARK_REGISTER_F(TelemFixture, alloc)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, free);
BENCHMARK_REGISTER_F(TelemFixture, free)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, msg);
BENCHMARK_REGISTER_F(TelemFixture, msg)->Threads(4);
BENCHMARK_REGISTER_F(TelemFixture, msg_printf);
BENCHMARK_REGISTER_F(TelemFixture, msg_printf)->Threads(4);


