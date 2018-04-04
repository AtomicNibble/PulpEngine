#pragma once

#ifndef X_POINTERUTIL_H_
#define X_POINTERUTIL_H_

#include "Casts\union_cast.h"

X_NAMESPACE_BEGIN(core)

namespace pointerUtil
{
    /// Aligns a pointer to the top of a given power-of-two alignment boundary.
    template<typename T>
    inline T* AlignTop(T* ptr, size_t alignment);

    /// Aligns a pointer to the bottom of a given power-of-two alignment boundary.
    template<typename T>
    inline T* AlignBottom(T* ptr, size_t alignment);

    template<typename T>
    inline bool IsAligned(T value, unsigned int alignment, unsigned int offset);

    template<typename T>
    inline bool IsAligned(T* value, unsigned int alignment, unsigned int offset);

} // namespace pointerUtil

#include "PointerUtil.inl"

X_NAMESPACE_END

#endif // X_POINTERUTIL_H_
