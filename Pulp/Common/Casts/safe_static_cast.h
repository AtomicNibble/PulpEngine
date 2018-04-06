#pragma once

#ifndef X_SAFE_STATIC_CAST_H_
#define X_SAFE_STATIC_CAST_H_

#include "Traits/UnsignedTypes.h"

X_NAMESPACE_BEGIN(core)

namespace internal
{
    /// Base template for casting from one type into another
    template<bool IsFromSigned, bool IsToSigned>
    struct safe_static_cast_helper;

} // namespace internal

X_NAMESPACE_END

template<typename TO, typename FROM>
X_INLINE TO safe_static_cast(FROM from)
{
#if X_ENABLE_SAFE_STATIC_CAST
    // delegate the call to the proper helper class, depending on the signedness of both types
    return X_NAMESPACE(core)::internal::safe_static_cast_helper<std::numeric_limits<FROM>::is_signed, std::numeric_limits<TO>::is_signed>::template cast<TO>(from);
#else
    return static_cast<TO>(from);
#endif
}

#include "safe_static_cast.inl"

#endif // X_SAFE_STATIC_CAST_H_
