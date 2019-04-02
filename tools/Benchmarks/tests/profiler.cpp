#include "stdafx.h"

void BM_telem_zone(benchmark::State& state)
{
    // i need to make this from something.
    const size_t telemBufSize = 1024 * 1024 * 32;
    static uint8_t telemBuf[telemBufSize];

    // Setup telem.
    X_ASSERT(ttLoadLibary(), "Failed to resolve telem")();
    X_ASSERT(ttInit(), "Failed to init telem")();

    telem::ContexHandle ctx;
    ttInitializeContext(ctx, telemBuf, sizeof(telemBuf));

    auto res = ttOpen(
        ctx,
        "Benchmarks",
        X_BUILD_STRING " Version: " X_ENGINE_VERSION_STR " Rev: " X_ENGINE_BUILD_REF_STR,
        "127.0.0.1",
        8001,
        telem::ConnectionType::Tcp,
        1000
    );

    X_ASSERT(res == telem::Error::Ok, "Failed to init")(res);

    ttSetThreadName(ctx, 0, "Main");

    ttTick(ctx);

    while (state.KeepRunning())
    {
        ttEnter(ctx, "meow");
        ttLeave(ctx);
    }

    ttFlush(ctx);
    ttClose(ctx);

    ttShutdownContext(ctx);
    ttShutDown();
}

BENCHMARK(BM_telem_zone);
