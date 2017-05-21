#pragma once

#ifndef X_SAFE_STATIC_CAST_H_
#define X_SAFE_STATIC_CAST_H_

#include "Traits/UnsignedTypes.h"


/// \ingroup Casts
/// \brief Safely static_casts from one type into the other taking into account signedness and numeric limits.
/// \details If the cast cannot be made, an assertion will fire.
/// \code
///   uint32_t integer = 65000;
///   uint16_t smaller = safe_static_cast<uint16_t>(integer);
/// \endcode
/// \remark Only works for integer types.
/// \sa union_cast X_ENABLE_SAFE_STATIC_CAST
template <typename TO, typename FROM>
X_INLINE TO safe_static_cast(FROM from)
{
#if X_ENABLE_SAFE_STATIC_CAST
	// delegate the call to the proper helper class, depending on the signedness of both types
	return X_NAMESPACE(core)::internal::safe_static_cast_helper<std::numeric_limits<FROM>::is_signed, std::numeric_limits<TO>::is_signed>:: template cast<TO>(from);
#else
	return static_cast<TO>(from);
#endif
}

#include "safe_static_cast.inl"


#endif // X_SAFE_STATIC_CAST_H_
