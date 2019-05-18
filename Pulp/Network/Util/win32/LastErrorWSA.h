#pragma once

X_NAMESPACE_BEGIN(net)

namespace lastErrorWSA
{
    typedef char Description[512];

    int32_t Get(void);

    X_NO_INLINE const char* ToString(int32_t error, Description& desc);
    X_NO_INLINE const char* ToString(Description& desc);

} // namespace lastErrorWSA

X_NAMESPACE_END
