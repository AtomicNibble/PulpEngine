#pragma once

#ifndef X_FLOAT_IEEE_UTIL_H_
#define X_FLOAT_IEEE_UTIL_H_

X_NAMESPACE_BEGIN(core)

namespace FloatUtil
{
    // info.
    static const int IEEE_FLT_MANTISSA_BITS = 23;
    static const int IEEE_FLT_EXPONENT_BITS = 8;
    static const int IEEE_FLT_EXPONENT_BIAS = 127;
    static const int IEEE_FLT_SIGN_BIT = 31;

    static const int SMALLEST_NON_DENORMAL = 1 << IEEE_FLT_MANTISSA_BITS;
    static const int NAN_VALUE = 0x7f800000;
    static const float FLT_SMALLEST_NON_DENORMAL = *reinterpret_cast<const float*>(&SMALLEST_NON_DENORMAL);

    X_INLINE bool isSignBitSet(float val)
    {
        uint32_t u = union_cast<uint32_t, float>(val);

        return core::bitUtil::IsBitSet<uint32_t>(u, IEEE_FLT_SIGN_BIT);
    }

    X_INLINE bool isSignBitNotSet(float val)
    {
        uint32_t u = union_cast<uint32_t, float>(val);

        return core::bitUtil::IsBitSet<uint32_t>(u, IEEE_FLT_SIGN_BIT) == false;
    }

} // namespace FloatUtil

X_NAMESPACE_END

#endif // !X_FLOAT_IEEE_UTIL_H_