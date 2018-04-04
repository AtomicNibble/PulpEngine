
#pragma once
#ifndef X_EXTRACTTYPE_H_
#define X_EXTRACTTYPE_H_

X_NAMESPACE_BEGIN(core)

namespace compileTime
{
    template<class T>
    struct ExtractType
    {
        typedef T Type;
    };

    template<class T, size_t N>
    struct ExtractType<T[N]>
    {
        typedef T Type;
    };

    template<typename T>
    struct IsFloat
    {
        static const bool Value = std::is_floating_point<T>::value;
    };

    template<typename T>
    struct IsSigned
    {
        static const bool Value = std::is_signed<T>::value;
    };

    template<typename T>
    struct IsUnsigned
    {
        static const bool Value = std::is_unsigned<T>::value;
    };

} // namespace compileTime

X_NAMESPACE_END

#endif // X_EXTRACTTYPE_H_
