#pragma once

#ifndef X_UTIL_ENDIAN_H_
#define X_UTIL_ENDIAN_H_

X_NAMESPACE_BEGIN(core)

namespace Endian
{
    template<typename T>
    X_INLINE T swap(T v);
}

#include "EndianUtil.inl"

X_NAMESPACE_END

#endif // X_UTIL_ENDIAN_H_
