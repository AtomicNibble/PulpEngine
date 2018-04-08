
X_NAMESPACE_BEGIN(core)


namespace atomic
{
    X_INTRINSIC(_InterlockedIncrement)
    X_INTRINSIC(_InterlockedDecrement)
    X_INTRINSIC(_InterlockedExchangeAdd)
    X_INTRINSIC(_InterlockedExchange)
    X_INTRINSIC(_InterlockedCompareExchange)

#if X_64
    X_INTRINSIC(_InterlockedIncrement64)
    X_INTRINSIC(_InterlockedDecrement64)
    X_INTRINSIC(_InterlockedExchangeAdd64)
    X_INTRINSIC(_InterlockedExchange64)
    X_INTRINSIC(_InterlockedCompareExchange64)
#endif // !X_64

    //	X_INTRINSIC (_InterlockedIncrement16)
    //	X_INTRINSIC (_InterlockedDecrement16)
    //	X_INTRINSIC (_InterlockedExchangeAdd16)
    //	X_INTRINSIC (_InterlockedExchange16)
    //	X_INTRINSIC (_InterlockedCompareExchange16)

    namespace internal
    {
        template<size_t N>
        struct Implementation
        {
        };

#if X_64

        /// Template specialization for 8-byte types.
        template<>
        struct Implementation<8u>
        {
            template<typename T>
            static inline T Increment(volatile T* memory)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedIncrement64(reinterpret_cast<volatile __int64*>(memory)));
            }

            template<typename T>
            static inline T Decrement(volatile T* memory)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedDecrement64(reinterpret_cast<volatile __int64*>(memory)));
            }

            template<typename T>
            static inline T Add(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(memory), static_cast<__int64>(value)));
            }

            template<typename T>
            static inline T Exchange(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedExchange64(reinterpret_cast<volatile __int64*>(memory), static_cast<__int64>(value)));
            }

            template<typename T>
            static inline T CompareExchange(volatile T* memory, T exchange, T comperand)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(memory),
                    static_cast<__int64>(exchange), static_cast<__int64>(comperand)));
            }
        };
#endif // !X_64

        /// Template specialization for 4-byte types.
        template<>
        struct Implementation<4u>
        {
            template<typename T>
            static inline T Increment(volatile T* memory)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedIncrement(reinterpret_cast<volatile long*>(memory)));
            }

            template<typename T>
            static inline T Decrement(volatile T* memory)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedDecrement(reinterpret_cast<volatile long*>(memory)));
            }

            template<typename T>
            static inline T Add(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(memory), static_cast<long>(value)));
            }

            template<typename T>
            static inline T Subtract(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(InterlockedExchangeSubtract(reinterpret_cast<volatile unsigned long*>(memory), static_cast<unsigned long>(value)));
            }

            template<typename T>
            static inline T And(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedAnd(reinterpret_cast<volatile long*>(memory), static_cast<long>(value)));
            }

            template<typename T>
            static inline T Or(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedOr(reinterpret_cast<volatile long*>(memory), static_cast<long>(value)));
            }

            template<typename T>
            static inline T Exchange(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedExchange(reinterpret_cast<volatile long*>(memory), static_cast<long>(value)));
            }

            template<typename T>
            static inline T CompareExchange(volatile T* memory, T exchange, T comperand)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedCompareExchange(reinterpret_cast<volatile long*>(memory),
                    static_cast<long>(exchange), static_cast<long>(comperand)));
            }
        };

        /// Template specialization for 2-byte types.
        template<>
        struct Implementation<2u>
        {
            template<typename T>
            static inline T Increment(volatile T* memory)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedIncrement16(reinterpret_cast<volatile short*>(memory)));
            }

            template<typename T>
            static inline T Decrement(volatile T* memory)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedDecrement16(reinterpret_cast<volatile short*>(memory)));
            }

            template<typename T>
            static inline T Add(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedExchangeAdd16(reinterpret_cast<volatile short*>(memory), static_cast<short>(value)));
            }

            template<typename T>
            static inline T Exchange(volatile T* memory, T value)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedExchange16(reinterpret_cast<volatile short*>(memory), static_cast<short>(value)));
            }

            template<typename T>
            static inline T CompareExchange(volatile T* memory, T exchange, T comperand)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                X_ASSERT_ALIGNMENT(memory, 4, 0);
                return static_cast<T>(_InterlockedCompareExchange16(reinterpret_cast<volatile short*>(memory),
                    static_cast<short>(exchange), static_cast<short>(comperand)));
            }
        };

    } // namespace internal

    template<typename T>
    X_INLINE T Increment(volatile T* memory)
    {
        return internal::Implementation<sizeof(T)>::Increment(memory);
    }

    template<typename T>
    X_INLINE T Decrement(volatile T* memory)
    {
        return internal::Implementation<sizeof(T)>::Decrement(memory);
    }

    template<typename T>
    X_INLINE T Add(volatile T* memory, T value)
    {
        return internal::Implementation<sizeof(T)>::Add(memory, value);
    }

    template<typename T>
    X_INLINE T Subtract(volatile T* memory, T value)
    {
        return internal::Implementation<sizeof(T)>::Subtract(memory, value);
    }

    template<typename T>
    X_INLINE T And(volatile T* memory, T value)
    {
        return internal::Implementation<sizeof(T)>::And(memory, value);
    }

    template<typename T>
    X_INLINE T Or(volatile T* memory, T value)
    {
        return internal::Implementation<sizeof(T)>::Or(memory, value);
    }

    template<typename T>
    X_INLINE T Exchange(volatile T* memory, T value)
    {
        return internal::Implementation<sizeof(T)>::Exchange(memory, value);
    }

    template<typename T>
    X_INLINE T CompareExchange(volatile T* memory, T exchange, T comperand)
    {
        return internal::Implementation<sizeof(T)>::CompareExchange(memory, exchange, comperand);
    }
} // namespace atomic

X_NAMESPACE_END
