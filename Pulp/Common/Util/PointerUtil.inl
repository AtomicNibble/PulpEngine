

namespace pointerUtil
{
    namespace internal
    {
        /// Generic alignment function used internally.
        X_INLINE void* AlignTop(void* ptr, size_t alignment)
        {
            union
            {
                void* as_void;
                uintptr_t as_uintptr_t;
            };

            const size_t mask = alignment - 1;
            as_void = ptr;
            as_uintptr_t += mask;
            as_uintptr_t &= ~mask;
            return as_void;
        }

        /// Generic alignment function used internally.
        X_INLINE void* AlignBottom(void* ptr, size_t alignment)
        {
            union
            {
                void* as_void;
                uintptr_t as_uintptr_t;
            };

            const size_t mask = alignment - 1;
            as_void = ptr;
            as_uintptr_t &= ~mask;
            return as_void;
        }

    } // namespace internal

    template<typename T>
    X_INLINE T* AlignTop(T* ptr, size_t alignment)
    {
        return union_cast<T*>(internal::AlignTop(union_cast<void*>(ptr), alignment));
    }

    template<typename T>
    X_INLINE T* AlignBottom(T* ptr, size_t alignment)
    {
        return union_cast<T*>(internal::AlignBottom(union_cast<void*>(ptr), alignment));
    }

    template<typename T>
    X_INLINE bool IsAligned(T value, unsigned int alignment, unsigned int offset)
    {
        return ((value + offset) % alignment) == 0;
    }

    template<typename T>
    X_INLINE bool IsAligned(T* value, unsigned int alignment, unsigned int offset)
    {
        return ((union_cast<uintptr_t>(value) + offset) % alignment) == 0;
    }

} // namespace pointerUtil
