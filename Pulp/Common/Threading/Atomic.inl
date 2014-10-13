
namespace atomic
{
	#pragma intrinsic (_InterlockedIncrement)
	#pragma intrinsic (_InterlockedDecrement)
	#pragma intrinsic (_InterlockedExchangeAdd)
	#pragma intrinsic (_InterlockedExchange)
	#pragma intrinsic (_InterlockedCompareExchange)

	X_INLINE int32_t Increment(volatile int32_t* memory)
	{
		X_ASSERT_ALIGNMENT(memory, 4, 0);
		return static_cast<int32_t>(_InterlockedIncrement(reinterpret_cast<volatile long*>(memory)));
	}

	X_INLINE int32_t Decrement(volatile int32_t* memory)
	{
		X_ASSERT_ALIGNMENT(memory, 4, 0);
		return static_cast<int32_t>(_InterlockedDecrement(reinterpret_cast<volatile long*>(memory)));
	}

	X_INLINE int32_t Add(volatile int32_t* memory, int32_t value)
	{
		X_ASSERT_ALIGNMENT(memory, 4, 0);
		return static_cast<int32_t>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(memory), static_cast<long>(value)));
	}

	X_INLINE int32_t Exchange(volatile int32_t* memory, int32_t value)
	{
		X_ASSERT_ALIGNMENT(memory, 4, 0);
		return static_cast<int32_t>(_InterlockedExchange(reinterpret_cast<volatile long*>(memory), static_cast<long>(value)));
	}

	X_INLINE int32_t CompareExchange(volatile int32_t* memory, int32_t exchange, int32_t comperand)
	{
		X_ASSERT_ALIGNMENT(memory, 4, 0);
		return static_cast<int32_t>(_InterlockedCompareExchange(reinterpret_cast<volatile long*>(memory), static_cast<long>(exchange), static_cast<long>(comperand)));
	}
}
