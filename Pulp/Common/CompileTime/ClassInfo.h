
#pragma once
#ifndef X_CLASSINFO_H
#define X_CLASSINFO_H

X_NAMESPACE_BEGIN(core)

namespace compileTime
{
    template<typename T>
    struct IsTrivial
    {
        static constexpr bool Value = std::is_trivial<T>::value;
    };

    template<typename T>
    struct IsCon
    {
        static constexpr bool Value = std::is_constructible<T>::value;
    };

    template<typename T>
    struct IsDefaultCon
    {
        static constexpr bool Value = std::is_default_constructible<T>::value;
    };

    template<typename T>
    struct IsCopyCon
    {
        static constexpr bool Value = std::is_copy_constructible<T>::value;
    };

    template<typename T>
    struct IsMoveCon
    {
        static constexpr bool Value = std::is_move_constructible<T>::value;
    };

    // trivial
    template<typename T>
    struct IsTrivialCon
    {
        static constexpr bool Value = std::is_trivially_constructible<T>::value;
    };

    template<typename T>
    struct IsTrivialDestruct
    {
        static constexpr bool Value = std::is_trivially_destructible<T>::value;
    };

    template<typename T>
    struct IsTrivialDefaultCon
    {
        static constexpr bool Value = std::is_trivially_default_constructible<T>::value;
    };

    template<typename T>
    struct IsTrivialCopy
    {
        static constexpr bool Value = std::is_trivially_copyable<T>::value;
    };

    template<typename T>
    struct IsTrivialCopyCon
    {
        static constexpr bool Value = std::is_trivially_copy_constructible<T>::value;
    };

    template<typename T>
    struct IsTrivialCopyAssign
    {
        static constexpr bool Value = std::is_trivially_copy_assignable<T>::value;
    };

    template<typename T>
    struct IsTrivialMoveCon
    {
        static constexpr bool Value = std::is_trivially_move_constructible<T>::value;
    };

    template<typename T>
    struct IsTrivialMoveAssign
    {
        static constexpr bool Value = std::is_trivially_move_assignable<T>::value;
    };

    template<typename T>
    struct HasVirtualDestructor
    {
        static constexpr bool Value = std::has_virtual_destructor<T>::value;
    };

    template<typename T>
    struct IsAbstract
    {
        static constexpr bool Value = std::is_abstract<T>::value;
    };

    template<typename T>
    struct IsFinal
    {
        static constexpr bool Value = std::is_final<T>::value;
    };

    template<typename T>
    struct IsPolymorphic
    {
        static constexpr bool Value = std::is_polymorphic<T>::value;
    };

} // namespace compileTime

X_NAMESPACE_END

#endif // X_CLASSINFO_H
