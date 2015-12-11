#include <EngineCommon.h>
#include "Fiber.h"


X_NAMESPACE_BEGIN(core)


namespace Fiber
{

	FiberHandle ConvertThreadToFiber(void)
	{
		return ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
	}

	void ConvertFiberToThread(void)
	{
		::ConvertFiberToThread();
	}

	FiberHandle CreateFiber(size_t stackCommitSize, size_t stackReserveSize, 
		FiberStartRoutine startRoutine, void* pArg)
	{
		FiberHandle fiber =::CreateFiberEx(stackCommitSize, stackReserveSize,
			FIBER_FLAG_FLOAT_SWITCH, startRoutine, pArg);

		if (!fiber)
		{
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



} // namespadce Fiber


X_NAMESPACE_END