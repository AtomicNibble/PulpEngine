#include <EngineCommon.h>
#include "Fiber.h"

X_NAMESPACE_BEGIN(core)

namespace Fiber
{
    FiberHandle ConvertThreadToFiber(void* pArg)
    {
        X_LOG2("Fiber", "Converting thread ^6%" PRIx32 "^7 to fiber", Thread::getCurrentID());

        FiberHandle fiber = ::ConvertThreadToFiberEx(pArg, FIBER_FLAG_FLOAT_SWITCH);

        if (!fiber) {
            lastError::Description Dsc;
            X_ERROR("Fiber", "Failed to create fiber. Err: %s", lastError::ToString(Dsc));
        }

        return fiber;
    }

    void ConvertFiberToThread(void)
    {
        X_LOG2("Fiber", "Converting fiber ^6%" PRIx32 "^7 to thread", Thread::getCurrentID());

        if (!::ConvertFiberToThread()) {
            lastError::Description Dsc;
            X_ERROR("Fiber", "Failed to convert fiber to thread. Err: %s", lastError::ToString(Dsc));
        }
    }

    FiberHandle CreateFiber(size_t stackCommitSize, size_t stackReserveSize,
        FiberStartRoutine startRoutine, void* pArg)
    {
        FiberHandle fiber = ::CreateFiberEx(stackCommitSize, stackReserveSize,
            FIBER_FLAG_FLOAT_SWITCH, startRoutine, pArg);

        if (!fiber) {
            lastError::Description Dsc;
            X_ERROR("Fiber", "Failed to create fiber. Err: %s", lastError::ToString(Dsc));
        }

        return fiber;
    }

    void DeleteFiber(FiberHandle fiber)
    {
        ::DeleteFiber(fiber);
    }

    void SwitchToFiber(FiberHandle destFiber)
    {
        ::SwitchToFiber(destFiber);
    }

    FiberHandle GetCurrentFiber(void)
    {
        return ::GetCurrentFiber();
    }

    void* GetFiberData(void)
    {
        return ::GetFiberData();
    }

} // namespace Fiber

X_NAMESPACE_END