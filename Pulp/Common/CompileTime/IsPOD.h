#pragma once

#ifndef X_ISPOD_H_
#define X_ISPOD_H_

#include <type_traits>

X_NAMESPACE_BEGIN(core)

namespace compileTime
{
    template<typename T>
    struct IsPOD
    {
        static const bool Value = std::is_pod<T>::value;
    };
} // namespace compileTime

X_NAMESPACE_END

#endif // !X_ISPOD_H_
