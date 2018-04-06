#include "EngineCommon.h"
#include "MultiplyWithCarry.h"

X_NAMESPACE_BEGIN(core)

namespace random
{
    MultiplyWithCarry::MultiplyWithCarry() :
        z_(0x159a55e5),
        w_(0x1f123bb5)
    {
    }

    MultiplyWithCarry::MultiplyWithCarry(const Vec4i& seed) :
        z_(seed[0] ^ seed[1]),
        w_(seed[2] ^ seed[3])
    {
    }

    void MultiplyWithCarry::setSeed(const Vec4i& seed)
    {
        z_ = seed[0] ^ seed[1];
        w_ = seed[2] ^ seed[3];
    }

} // namespace random

X_NAMESPACE_END