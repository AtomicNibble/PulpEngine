#include "stdafx.h"

void BM_telem_zone(benchmark::State& state)
{
    // i need to make this from something.
    const size_t telemBufSize = 1024 * 1024 * 2;
    static uint8_t telemBuf[telemBufSize];

    // Setup telem.
    X_ASSERT(ttLoadLibary(), "Failed to resolve telem")();
    X_ASSERT(ttInit(), "Failed to init telem")();

    telem::ContexHandle ctx;
    ttInitializeContext(ctx, telemBuf, sizeof(telemBuf));
    ttSetThreadName(ctx, 0, "Main");

    ttTick(ctx);

    while (state.KeepRunning())
    {
        ttEnter(ctx, "meow");
        ttLeave(ctx);
    }

    ttClose(ctx);

    ttShutdownContext(ctx);
    ttShutDown();
}

void BM_telem_zone_printf(benchmark::State& state)
{
    const size_t telemBufSize = 1024 * 1024 * 2;
    static uint8_t telemBuf[telemBufSize];

    // Setup telem.
    X_ASSERT(ttLoadLibary(), "Failed to resolve telem")();
    X_ASSERT(ttInit(), "Failed to init telem")();

    telem::ContexHandle ctx;
    ttInitializeContext(ctx, telemBuf, sizeof(telemBuf));
    ttSetThreadName(ctx, 0, "Main");

    ttTick(ctx);

    while (state.KeepRunning())
    {
        ttEnter(ctx, "this is a string %s in the middle", "with format spec");
        ttLeave(ctx);
    }

    ttClose(ctx);

    ttShutdownContext(ctx);
    ttShutDown();
}

BENCHMARK(BM_telem_zone);
BENCHMARK(BM_telem_zone_printf);
