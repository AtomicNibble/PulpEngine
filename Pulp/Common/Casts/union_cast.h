#pragma once

#ifndef X_UNION_CAST_H_
#define X_UNION_CAST_H_

/// \ingroup Casts
/// \brief Casts from one type into another via a union.
/// \details This type of cast is similar to a reinterpret_cast, but never violates the strict aliasing rule (which
/// reinterpret_cast does), and should therefore be used e.g. when converting from pointer-types to integer-types, and
/// vice versa.
/// \code
///   void* ptr = ...;
///   uintptr_t asInteger = union_cast<uintptr_t>(ptr);
/// \endcode
/// \remark Both types must have the same size.
/// \sa safe_static_cast
template<typename TO, typename FROM>
inline TO union_cast(FROM from);

#include "union_cast.inl"

#endif // X_UNION_CAST_H_
