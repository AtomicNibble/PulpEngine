

namespace Endian
{
    namespace internal
    {
        template<size_t N>
        struct Implementation
        {
        };

        /// Template specialization for 8-byte types.
        template<>
        struct Implementation<8u>
        {
            template<typename T>
            static inline T swap(T v)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                return (T)_byteswap_uint64(v);
            }
        };

        /// Template specialization for 4-byte types.
        template<>
        struct Implementation<4u>
        {
            template<typename T>
            static inline T swap(T v)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                return (T)_byteswap_ulong(v);
            }
        };

        /// Template specialization for 2-byte types.
        template<>
        struct Implementation<2u>
        {
            template<typename T>
            static inline T swap(T v)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                return (T)_byteswap_ushort(v);
            }
        };

    } // namespace internal

    template<typename T>
    X_INLINE T swap(T v)
    {
        return internal::Implementation<sizeof(T)>::swap(v);
    }

} // namespace Endian