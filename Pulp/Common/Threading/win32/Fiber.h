#pragma once

X_NAMESPACE_BEGIN(core)

namespace Fiber
{
    typedef void(__stdcall* FiberStartRoutine)(void* arg);
    typedef void* FiberHandle;

    constexpr FiberHandle InvalidFiberHandle = nullptr;

    FiberHandle ConvertThreadToFiber(void* pArg);

    void ConvertFiberToThread(void);
    FiberHandle CreateFiber(size_t stackCommitSize, size_t stackReserveSize,
        FiberStartRoutine startRoutine, void* pArg);
    void DeleteFiber(FiberHandle fiber);

    void SwitchToFiber(FiberHandle destFiber);
    FiberHandle GetCurrentFiber(void);

    void* GetFiberData(void);

} // namespace Fiber

X_NAMESPACE_END