#pragma once

#ifndef X_BITUTIL_H_
#define X_BITUTIL_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Util
/// \namespace bitUtil
/// \brief Provides bit manipulation routines.
/// \details The functions in this namespace can be used to count, set, clear, and replace bits. Additionally, it
/// contains routines for dealing with and rounding to power-of-two values.
namespace bitUtil
{
    // used for selecting a type that can hold
    // any flag that Type T can hold.
    namespace FlagType
    {
        template<typename T>
        struct return_
        {
            typedef T type;
        };

        template<uint64_t N>
        struct bytetype : return_<uint64_t>
        {
        };

        template<>
        struct bytetype<4> : return_<uint32_t>
        {
        };

        template<>
        struct bytetype<3> : return_<uint32_t>
        {
        };

        template<>
        struct bytetype<2> : return_<uint16_t>
        {
        };

        template<>
        struct bytetype<1> : return_<uint8_t>
        {
        };

    } // namespace FlagType

    /// A constant for signaling that no bit is set in a value passed to ScanBits().
    static const unsigned int NO_BIT_SET = 255;

    /// Returns whether a flag is set in the given integer value.
    template<typename T>
    inline bool IsBitFlagSet(T value, typename FlagType::bytetype<sizeof(T)>::type flag);

    /// Clears a flag in an integer
    template<typename T>
    inline T ClearBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag);

    /// Sets a flag in an integer
    template<typename T>
    inline T SetBitFlag(T value, typename FlagType::bytetype<sizeof(T)>::type flag);

    /// Returns whether a certain bit is set in the given integer value.
    template<typename T>
    inline bool IsBitSet(T value, unsigned int whichBit);

    /// Returns whether a certain bit is set in the given integer value.
    template<typename T>
    inline bool IsBitNotSet(T value, unsigned int whichBit);

    /// Sets a bit in an integer.
    template<typename T>
    inline T SetBit(T value, unsigned int whichBit);

    /// Clears a bit in an integer.
    template<typename T>
    inline T ClearBit(T value, unsigned int whichBit);

    /// Replaces bits in an integer.
    template<typename T>
    inline T ReplaceBits(T value, unsigned int startBit, unsigned int howMany, T bits);

    /// Returns the number of bits set in an integer.
    template<typename T>
    inline unsigned int CountBits(T value);

    /// \brief Returns the position of the first set bit found, scanning from the MSB to the LSB (e.g. 0x1 returns 0, 0x80000000 returns 31).
    /// \remark Returns NO_BIT_SET if no bit is set in the given \a value.
    template<typename T>
    inline unsigned int ScanBits(T value);

    /// \brief Returns the position of the first set bit found, scanning from the LSB to the MSB (e.g. 0x1 returns 0, 0x80000000 returns 31).
    /// \remark Returns NO_BIT_SET if no bit is set in the given \a value.
    template<typename T>
    inline unsigned int ScanBitsForward(T value);

    /// \brief Returns whether a given integer is a power-of-two.
    /// \remark 0 is considered to be a power-of-two in this case.
    template<typename T>
    inline constexpr bool IsPowerOfTwo(T x);

    /// Returns the next power-of-two for a given value.
    template<typename T>
    inline constexpr T NextPowerOfTwo(T v);

    /// Rounds a number up to the next multiple of a power-of-two.
    template<typename T>
    inline constexpr T RoundUpToMultiple(T numToRound, T multipleOf);

    /// Rounds a number down to the next multiple of a power-of-two.
    inline uint32_t RoundDownToMultiple(uint32_t numToRound, uint32_t multipleOf);

    // TODO: finish unit tests.
    // sign util
    template<typename T>
    inline bool isSignBitSet(T value);

    // sign util
    template<typename T>
    inline bool isSignBitNotSet(T value);

    // these are just for clarity in code.
    template<typename T>
    constexpr T bitsToBytes(T numBits);
    template<typename T>
    constexpr T bytesToBits(T numBytes);

    // how many bits are needed to store this value?
    template<typename T>
    constexpr T bitsNeededForValue(T n);

    // turns alpha char's into bit indexes.
    // a = 6
    // b = 7
    // z = 31
    constexpr inline uint32_t AlphaBit(char c);

    constexpr inline char BitToAlphaChar(uint32_t bit);

    inline uint32_t AlphaBits(const char* pStr);

} // namespace bitUtil

#include "BitUtil.inl"

X_NAMESPACE_END

#endif
