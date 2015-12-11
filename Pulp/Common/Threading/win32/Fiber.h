#pragma once


X_NAMESPACE_BEGIN(core)


namespace Fiber
{
	typedef void (__stdcall *FiberStartRoutine)(void* arg);
	typedef void* FiberHandle;


	FiberHandle ConvertThreadToFiber(void);

	void ConvertFiberToThread(void);
	FiberHandle CreateFiber(size_t stackCommitSize, size_t stackReserveSize,
		FiberStartRoutine startRoutine, void* pArg);
	inline void DeleteFiber(FiberHandle fiber);

	void SwitchToFiber(FiberHandle destFiber);
	FiberHandle GetCurrentFiber(void);

} // namespadce Fiber


X_NAMESPACE_END