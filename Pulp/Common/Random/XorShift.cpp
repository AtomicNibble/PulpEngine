#include "EngineCommon.h"
#include "XorShift.h"

X_NAMESPACE_BEGIN(core)

namespace random
{
    XorShift::XorShift() :
        state_(
            123456789, 362436069, 521288629, 88675123)
    {
    }

    XorShift::XorShift(const Vec4i& seed) :
        state_(seed)
    {
    }

    void XorShift::setSeed(const Vec4i& seed)
    {
        state_ = seed;
    }

    // -------------------------------------------

    XorShift128::XorShift128() :
        state_{0x53A0CE0035EB8F8, 0x86682309152AAAC5}
    {
    }

    XorShift128::XorShift128(const Vec4i& seed)
    {
        setSeed(seed);
    }

    void XorShift128::setSeed(const Vec4i& seed)
    {
        state_[0] = (static_cast<uint64_t>(seed.x) << 32 | static_cast<uint64_t>(seed.y));
        state_[1] = (static_cast<uint64_t>(seed.z) << 32 | static_cast<uint64_t>(seed.w));
    }

} // namespace random

X_NAMESPACE_END