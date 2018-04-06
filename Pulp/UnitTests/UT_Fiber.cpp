#include "stdafx.h"

#include "Threading\Fiber.h"

X_USING_NAMESPACE;

using namespace core;

namespace
{
    static size_t FIBER_STACK_SIZE = 2048;
    static const size_t NUM_FIBERS = 10;
    static const size_t PRIMARY_FIBER_IDX = NUM_FIBERS;
    FixedArray<Fiber::FiberHandle, NUM_FIBERS> g_fibers;

    size_t g_currentFiber = 0;

    void __stdcall FiberStart(void* pArg)
    {
        size_t param = reinterpret_cast<size_t>(pArg);

        Fiber::FiberHandle curFiber = Fiber::GetCurrentFiber();

        EXPECT_EQ(g_fibers[g_currentFiber], curFiber);
        EXPECT_EQ(g_currentFiber, param);

        X_LOG0("Fiber", "In fiber: %i", param);

        ++g_currentFiber;

        SwitchToFiber(g_fibers[g_currentFiber]);
    }

} // namespace

TEST(Threading, Fiber)
{
    // make some fibers with diffrent data values.
    for (size_t i = 0; i < NUM_FIBERS - 1; i++) {
        Fiber::FiberHandle newFiber = Fiber::CreateFiber(FIBER_STACK_SIZE, FIBER_STACK_SIZE, FiberStart,
            reinterpret_cast<void*>(i));
        g_fibers.append(newFiber);
    }

    // create a fiber for this thread to switch back to after we done.
    Fiber::FiberHandle mainThreadFiber = Fiber::ConvertThreadToFiber(nullptr);
    g_fibers.append(mainThreadFiber);

    SwitchToFiber(g_fibers[g_currentFiber]);

    Fiber::ConvertFiberToThread();

    // don't delete last fiber as it got deleted by ConvertFiberToThread above.
    for (size_t i = 0; i < NUM_FIBERS - 1; i++) {
        Fiber::DeleteFiber(g_fibers[i]);
    }
}