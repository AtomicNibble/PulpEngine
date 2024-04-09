#pragma once

#ifndef X_UNION_CAST_H_
#define X_UNION_CAST_H_

template<typename TO, typename FROM>
inline TO union_cast(FROM from);

#include "union_cast.inl"

#endif // X_UNION_CAST_H_
