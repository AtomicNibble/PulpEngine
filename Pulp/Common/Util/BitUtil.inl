
/// \ingroup Util
namespace bitUtil
{
    // tell the compiler to use the intrinsic versions of these functions
    X_INTRINSIC(_BitScanReverse)
    X_INTRINSIC(_BitScanForward)

#if X_64
    X_INTRINSIC(_BitScanReverse64)
    X_INTRINSIC(_BitScanForward64)
#endif // !X_64

    /// \ingroup Util
    namespace internal
    {
        /// \brief Base template for types of size N.
        /// \details The individual template specializations take care of working with the correct unsigned type
        /// based on the size of the generic type T. First, this allows for optimizations based on the size of a given
        /// type (counting bits for 16-bit types differs from counting bits for 32-bit types). Second, the implementations can make
        /// sure to only use unsigned types when working with individual bits. Signed types should be avoided for bit
        /// manipulation because some operations like e.g. a right-shift are implementation-defined for signed types
        /// in C++.
        template<size_t N>
        struct Implementation
        {
        };

        /// Template specialization for 4-byte types.
        template<>
        struct Implementation<8u>
        {
            template<typename T>
            static inline bool IsBitFlagSet(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                static_assert(sizeof(flag) == 8, "sizeof(flag) is not 8 bytes.");

                return (static_cast<uint64_t>(value) & flag) == flag;
            }

            template<typename T>
            static inline T ClearBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                static_assert(sizeof(flag) == 8, "sizeof(flag) is not 8 bytes.");

                return (static_cast<uint64_t>(value) & (~flag));
            }

            template<typename T>
            static inline T SetBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");
                static_assert(sizeof(flag) == 8, "sizeof(flag) is not 8 bytes.");

                return (static_cast<uint64_t>(value) | flag);
            }

            /// Internal function used by bitUtil::RoundUpToMultiple.
            template<typename T>
            static inline constexpr uint64_t RoundUpToMultiple(T numToRound, T multipleOf)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
            }

            /// Internal function used by bitUtil::CountBits.
            template<typename T>
            static inline unsigned int CountBits(T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                uint64_t v = static_cast<uint64_t>(value) - ((static_cast<uint64_t>(value) >> 1ui64) & 0x5555555555555555ui64);
                v = (v & 0x3333333333333333ui64) + ((v >> 2ui64) & 0x3333333333333333ui64);
                return (((v + (v >> 4ui64) & 0xF0F0F0F0F0F0F0Fui64) * 0x101010101010101ui64) >> 56ui64);
            }

            /// Internal function used by bitUtil::ScanBits.
            /// index of MSB
            template<typename T>
            static inline unsigned int ScanBits(T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

#if X_64
                unsigned long index = 0;

                const unsigned char result = _BitScanReverse64(&index, static_cast<uint64_t>(value));
                if (result == 0) {
                    return NO_BIT_SET;
                }

                return index;
#else
                static const unsigned int bval[] = {
                    0,
                    1,
                    2, 2,
                    3, 3, 3, 3,
                    4, 4, 4, 4, 4, 4, 4, 4};

                if (value == 0) {
                    return NO_BIT_SET;
                }

                unsigned int r = 0;

                if (value & 0xFFFFFFFF00000000) {
                    r += 32 / 1;
                    value >>= 32 / 1;
                }
                if (value & 0x00000000FFFF0000) {
                    r += 16 / 1;
                    value >>= 16 / 1;
                }
                if (value & 0x000000000000FF00) {
                    r += 16 / 2;
                    value >>= 16 / 2;
                }
                if (value & 0x00000000000000F0) {
                    r += 16 / 4;
                    value >>= 16 / 4;
                }
                return r + bval[value] - 1;
#endif // !X_64
            }

            /// index of LSB
            template<typename T>
            static inline unsigned int ScanBitsForward(T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                unsigned long index = 0;
#if X_64
                const unsigned char result = _BitScanForward64(&index, static_cast<uint64_t>(value));
                if (result == 0) {
                    return NO_BIT_SET;
                }

                return index;
#else
                if (value) {
                    value = (value ^ (value - 1)) >> 1; // Set v's trailing 0s to 1s and zero rest
                    for (index = 0; value; index++) {
                        value >>= 1;
                    }
                    return index;
                }
                return NO_BIT_SET;
#endif // !X_64
            }

            /// Internal function used by bitUtil::SetBit.
            template<typename T>
            static inline T SetBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                return (static_cast<T>(static_cast<uint64_t>(value) | (1ui64 << whichBit)));
            }

            /// Internal function used by bitUtil::ClearBit.
            template<typename T>
            static inline T ClearBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                return (static_cast<T>(static_cast<uint64_t>(value) & (~(1ui64 << whichBit))));
            }

            /// Internal function used by bitUtil::ReplaceBits.
            template<typename T>
            static inline T ReplaceBits(T value, unsigned int startBit, unsigned int howMany, T bits)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                const uint64_t mask = ~(((1ui64 << howMany) - 1ui64) << startBit);
                uint64_t cappedBits = howMany == 64 ? bits : static_cast<uint64_t>(bits) & ((1ui64 << howMany) - 1ui64);
                return (static_cast<T>((static_cast<uint64_t>(value) & mask) | (cappedBits << startBit)));
            }

            /// Internal function used by bitUtil::IsBitSet.
            template<typename T>
            static inline bool IsBitSet(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                return ((static_cast<uint64_t>(value) & (1ui64 << whichBit)) != 0);
            }

            template<typename T>
            static inline constexpr T NextPowerOfTwo(T v)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                --v;
                v |= v >> 1;
                v |= v >> 2;
                v |= v >> 4;
                v |= v >> 8;
                v |= v >> 16;
                v |= v >> 32;
                ++v;
                return v;
            }

            template<typename T>
            static inline bool isSignBitSet(T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                return IsBitSet<T>(value, 63);
            }

            template<typename T>
            static inline bool isSignBitNotSet(T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                return IsBitSet<T>(value, 63) == false;
            }
        };

        /// Template specialization for 4-byte types.
        template<>
        struct Implementation<4u>
        {
            /// Internal function used by bitUtil::IsBitFlagSet.
            template<typename T>
            static inline bool IsBitFlagSet(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                static_assert(sizeof(flag) == 4, "sizeof(flag) is not 4 bytes.");

                return (static_cast<uint32_t>(value) & flag) == flag;
            }

            template<typename T>
            static inline T ClearBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                static_assert(sizeof(flag) == 4, "sizeof(flag) is not 4 bytes.");

                return (static_cast<uint32_t>(value) & (~flag));
            }

            template<typename T>
            static inline T SetBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");
                static_assert(sizeof(flag) == 4, "sizeof(flag) is not 4 bytes.");

                return (static_cast<uint32_t>(value) | flag);
            }

            /// Internal function used by bitUtil::IsBitSet.
            template<typename T>
            static inline bool IsBitSet(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                return ((static_cast<uint32_t>(value) & (1u << whichBit)) != 0);
            }

            /// Internal function used by bitUtil::SetBit.
            template<typename T>
            static inline T SetBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                return (static_cast<T>(static_cast<uint32_t>(value) | (1u << whichBit)));
            }

            /// Internal function used by bitUtil::ClearBit.
            template<typename T>
            static inline T ClearBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                return (static_cast<T>(static_cast<uint32_t>(value) & (~(1u << whichBit))));
            }

            /// Internal function used by bitUtil::ReplaceBits.
            template<typename T>
            static inline T ReplaceBits(T value, unsigned int startBit, unsigned int howMany, T bits)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                const uint32_t mask = ~(((1u << howMany) - 1u) << startBit);
                uint32_t cappedBits = howMany == 32 ? bits : static_cast<uint32_t>(bits) & ((1u << howMany) - 1u);
                return (static_cast<T>((static_cast<uint32_t>(value) & mask) | (cappedBits << startBit)));
            }

            /// Internal function used by bitUtil::CountBits.
            template<typename T>
            static inline unsigned int CountBits(T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                uint32_t v = static_cast<uint32_t>(value) - ((static_cast<uint32_t>(value) >> 1u) & 0x55555555u);
                v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u);
                return (((v + (v >> 4u) & 0xF0F0F0Fu) * 0x1010101u) >> 24u);
            }

            /// Internal function used by bitUtil::ScanBits.
            template<typename T>
            static inline unsigned int ScanBits(T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                unsigned long index = 0;
                const unsigned char result = _BitScanReverse(&index, static_cast<DWORD>(value));
                if (result == 0) {
                    return NO_BIT_SET;
                }

                return index;
            }

            template<typename T>
            static inline unsigned int ScanBitsForward(T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                unsigned long index = 0;
                const unsigned char result = _BitScanForward(&index, static_cast<DWORD>(value));
                if (result == 0) {
                    return NO_BIT_SET;
                }

                return index;
            }

            /// Internal function used by bitUtil::RoundUpToMultiple.
            template<typename T>
            static inline constexpr unsigned int RoundUpToMultiple(T numToRound, T multipleOf)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
            }

            template<typename T>
            static inline constexpr T NextPowerOfTwo(T v)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                --v;
                v |= v >> 1;
                v |= v >> 2;
                v |= v >> 4;
                v |= v >> 8;
                v |= v >> 16;
                ++v;

                return v;
            }

            template<typename T>
            static inline bool isSignBitSet(T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                return IsBitSet<T>(value, 31);
            }

            template<typename T>
            static inline bool isSignBitNotSet(T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                return IsBitSet<T>(value, 31) == false;
            }
        };

        template<>
        struct Implementation<2u>
        {
            template<typename T>
            static inline bool IsBitFlagSet(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                static_assert(sizeof(flag) == 2, "sizeof(flag) is not 2 bytes.");

                return (static_cast<uint16_t>(value) & flag) == flag;
            }

            template<typename T>
            static inline T ClearBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                static_assert(sizeof(flag) == 2, "sizeof(flag) is not 2 bytes.");

                return (static_cast<uint16_t>(value) & (~flag));
            }

            template<typename T>
            static inline T SetBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");
                static_assert(sizeof(flag) == 2, "sizeof(flag) is not 2 bytes.");

                return (static_cast<uint16_t>(value) | flag);
            }

            /// Internal function used by bitUtil::IsBitSet.
            template<typename T>
            static inline bool IsBitSet(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

                return ((static_cast<uint16_t>(value) & (1u << whichBit)) != 0);
            }

            /// Internal function used by bitUtil::SetBit.
            template<typename T>
            static inline T SetBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

                return (static_cast<T>(static_cast<uint16_t>(value) | (1u << whichBit)));
            }

            /// Internal function used by bitUtil::ClearBit.
            template<typename T>
            static inline T ClearBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

                return (static_cast<T>(static_cast<uint16_t>(value) & (~(1u << whichBit))));
            }

            template<typename T>
            static inline constexpr T RoundUpToMultiple(T numToRound, T multipleOf)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

                return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
            }

            template<typename T>
            static inline constexpr T NextPowerOfTwo(T v)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

                --v;
                v |= v >> 1;
                v |= v >> 2;
                v |= v >> 4;
                v |= v >> 8;
                ++v;

                return v;
            }

            template<typename T>
            static inline bool isSignBitSet(T value)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

                return IsBitSet<T>(value, 15);
            }

            template<typename T>
            static inline bool isSignBitNotSet(T value)
            {
                static_assert(sizeof(T) == 2, "sizeof(T) is not 2 bytes.");

                return IsBitSet<T>(value, 15) == false;
            }
        };

        template<>
        struct Implementation<1u>
        {
            template<typename T>
            static inline bool IsBitFlagSet(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");
                static_assert(sizeof(flag) == 1, "sizeof(flag) is not 1 bytes.");

                return (static_cast<uint8_t>(value) & flag) == flag;
            }

            template<typename T>
            static inline T ClearBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");
                static_assert(sizeof(flag) == 1, "sizeof(flag) is not 1 bytes.");

                return (static_cast<uint8_t>(value) & (~flag));
            }

            template<typename T>
            static inline T SetBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");
                static_assert(sizeof(flag) == 1, "sizeof(flag) is not 1 bytes.");

                return (static_cast<uint8_t>(value) | flag);
            }

            /// Internal function used by bitUtil::IsBitSet.
            template<typename T>
            static inline bool IsBitSet(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");

                return ((static_cast<uint8_t>(value) & (1u << whichBit)) != 0);
            }

            /// Internal function used by bitUtil::SetBit.
            template<typename T>
            static inline T SetBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");

                return (static_cast<T>(static_cast<uint8_t>(value) | (1u << whichBit)));
            }

            /// Internal function used by bitUtil::ClearBit.
            template<typename T>
            static inline T ClearBit(T value, unsigned int whichBit)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");

                return (static_cast<T>(static_cast<uint8_t>(value) & (~(1u << whichBit))));
            }

            template<typename T>
            static inline bool isSignBitSet(T value)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");

                return IsBitSet<T>(value, 7);
            }

            template<typename T>
            static inline bool isSignBitNotSet(T value)
            {
                static_assert(sizeof(T) == 1, "sizeof(T) is not 1 bytes.");

                return IsBitSet<T>(value, 7) == false;
            }
        };
    } // namespace internal

    template<typename T>
    inline bool IsBitFlagSet(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
    {
        return internal::Implementation<sizeof(T)>::IsBitFlagSet(value, flag);
    }

    template<typename T>
    inline T ClearBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::ClearBitFlag(value, flag);
    }

    template<typename T>
    inline T SetBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::SetBitFlag(value, flag);
    }

    template<typename T>
    inline bool IsBitSet(T value, unsigned int whichBit)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::IsBitSet(value, whichBit);
    }

    template<typename T>
    inline bool IsBitNotSet(T value, unsigned int whichBit)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::IsBitSet(value, whichBit) == false;
    }

    template<typename T>
    inline T SetBit(T value, unsigned int whichBit)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::SetBit(value, whichBit);
    }

    template<typename T>
    inline T ClearBit(T value, unsigned int whichBit)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::ClearBit(value, whichBit);
    }

    template<typename T>
    inline T ReplaceBits(T value, unsigned int startBit, unsigned int howMany, T bits)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::ReplaceBits(value, startBit, howMany, bits);
    }

    template<typename T>
    inline unsigned int CountBits(T value)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::CountBits(value);
    }

    template<typename T>
    inline unsigned int ScanBits(T value)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::ScanBits(value);
    }

    template<typename T>
    inline unsigned int ScanBitsForward(T value)
    {
        // defer the implementation to the correct helper template, based on the size of the type
        return internal::Implementation<sizeof(T)>::ScanBitsForward(value);
    }

    template<typename T>
    inline constexpr bool IsPowerOfTwo(T x)
    {
        return (x & (x - 1)) == 0;
    }

    template<typename T>
    inline constexpr T NextPowerOfTwo(T v)
    {
        return internal::Implementation<sizeof(T)>::NextPowerOfTwo(v);
    }

    template<typename T>
    inline constexpr T RoundUpToMultiple(T numToRound, T multipleOf)
    {
        return internal::Implementation<sizeof(T)>::RoundUpToMultiple(numToRound, multipleOf);
    }

    inline uint32_t RoundDownToMultiple(uint32_t numToRound, uint32_t multipleOf)
    {
        return numToRound & ~(multipleOf - 1);
    }

    // sign util
    template<typename T>
    inline bool isSignBitSet(T value)
    {
        return internal::Implementation<sizeof(T)>::isSignBitSet(value);
    }

    // sign util
    template<typename T>
    inline bool isSignBitNotSet(T value)
    {
        return internal::Implementation<sizeof(T)>::isSignBitNotSet(value);
    }

    template<typename T>
    inline constexpr T bitsToBytes(T numBits)
    {
        return (numBits + 7) >> 3;
    }

    template<typename T>
    inline constexpr T bytesToBits(T numBytes)
    {
        return numBytes << 3;
    }

    template<typename T>
    inline constexpr T bitsNeededForValue(T n)
    {
        return n <= 1 ? 1 : 1 + bitsNeededForValue((n + 1) / 2);
    }

    // turns alpha char's into bit indexes.
    // a = 6
    // b = 7
    // z = 31
    constexpr inline uint32_t AlphaBit(char c)
    {
        return c >= 'a' && c <= 'z' ? 1 << (c - 'z' + 31) : 0;
    }

    constexpr inline char BitToAlphaChar(uint32_t bit)
    {
        // index 6 is 'a'
        return static_cast<char>(bit < 32 ? ('a' + bit) - 6 : '0');
    }

    inline uint32_t AlphaBits(const char* pStr)
    {
        uint32 val = 0;

        while (*pStr) {
            val |= AlphaBit(*pStr++);
        }

        return val;
    }

} // namespace bitUtil
