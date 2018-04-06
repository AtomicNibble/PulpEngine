#pragma once

#ifndef X_FUNCTIONTRAITS_H_
#define X_FUNCTIONTRAITS_H_

#include "Traits/FunctionSignatureTraits.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Traits
namespace traits
{
    /// \ingroup Traits
    namespace internal
    {
        /// Internal struct used by \ref traits::Function.
        template<typename R>
        struct Function : public FunctionSignature<R>
        {
            typedef R (*Pointer)(void);
        };

        /// Internal struct used by \ref traits::Function.
        template<typename R, typename A0>
        struct Function0 : public FunctionSignature0<R, A0>
        {
            typedef R (*Pointer)(A0);
        };

        /// Internal struct used by \ref traits::Function.
        template<typename R, typename A0, typename A1>
        struct Function1 : public FunctionSignature1<R, A0, A1>
        {
            typedef R (*Pointer)(A0, A1);
        };

        /// Internal struct used by \ref traits::Function.
        template<typename R, typename A0, typename A1, typename A2>
        struct Function2 : public FunctionSignature2<R, A0, A1, A2>
        {
            typedef R (*Pointer)(A0, A1, A2);
        };

        /// Internal struct used by \ref traits::Function.
        template<typename R, typename A0, typename A1, typename A2, typename A3>
        struct Function3 : public FunctionSignature3<R, A0, A1, A2, A3>
        {
            typedef R (*Pointer)(A0, A1, A2, A3);
        };

        /// Internal struct used by \ref traits::Function.
        template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
        struct Function4 : public FunctionSignature4<R, A0, A1, A2, A3, A4>
        {
            typedef R (*Pointer)(A0, A1, A2, A3, A4);
        };

        /// Internal struct used by \ref traits::Function.
        template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
        struct Function5 : public FunctionSignature5<R, A0, A1, A2, A3, A4, A5>
        {
            typedef R (*Pointer)(A0, A1, A2, A3, A4, A5);
        };
    } // namespace internal

    /// \ingroup Traits
    /// \brief A traits class for functions with arbitrary signature.
    /// \details This class template helps with declaring pointers-to-functions in a meaningful and descriptive
    /// way by providing template specializations and a corresponding typedef for functions with arbitrary signature
    /// (up to 6 arguments).
    ///
    /// Using the provided template classes, declaring pointers-to-functions is easy, as shown in the following example:
    /// \code
    ///   // declares a function taking an int and returning nothing
    ///   typedef core::traits::Function<void (int)> MyFunction;
    ///
    ///   // declares a pointer-to-function taking an int and returning nothing
    ///   typedef MyFunction::Pointer PointerToMyFunction;
    /// \endcode
    /// \code
    ///   // declares a function taking a char and an int, returning a bool
    ///   typedef core::traits::Function<bool (char, int)> MyFunction;
    ///
    ///   // declares a pointer-to-function taking a char and an int, returning a bool
    ///   typedef MyFunction::Pointer PointerToMyFunction;
    /// \endcode
    /// Furthermore, the class template helps in declaring return values and argument types from deduced
    /// template arguments. Each function template defines the following nested types:
    /// - ReturnType: the type of the return value
    /// - Arg0-Arg5: the types of the arguments (up to 6 are supported)
    ///
    /// This feature is especially useful when forwarding arguments to other functions, e.g. when interfacing with
    /// scripting languages. Additionally, it works for both explicit declarations as well as function pointers:
    /// \code
    ///   // explicit declaration. the template arguments are of the form "R (A0, A1, A2)"
    ///   typedef core::traits::Function<void (int, char, float)> MyFunction;
    ///   typedef MyFunction::ReturnType MyType;
    ///   typedef MyFunction::Arg0 ArgumentType0;
    ///   typedef MyFunction::Arg1 ArgumentType1;
    ///   typedef MyFunction::Arg2 ArgumentType2;
    ///   typedef MyFunction::Pointer PointerToMyFunction;
    ///
    ///   // function template accepting any pointer-to-function
    ///   template <typename F>
    ///   void Test(F function)
    ///   {
    ///     // F can be any function having 3 parameters.
    ///     // the template argument F to core::traits::Function<F> is of the form "R (*)(A0, A1, A2)"
    ///     typename core::traits::Function<F>::Arg0 a;
    ///     typename core::traits::Function<F>::Arg1 b;
    ///     typename core::traits::Function<F>::Arg2 c;
    ///   }
    /// \endcode
    /// \sa MemberFunction
    template<typename T>
    struct Function
    {
    };

    /// \brief Partial template specialization for functions having no arguments.
    /// \sa Function
    template<typename R>
    struct Function<R(void)> : public internal::Function<R>
    {
    };

    /// \brief Partial template specialization for functions having no arguments.
    /// \sa Function
    template<typename R>
    struct Function<R (*)(void)> : public internal::Function<R>
    {
    };

    /// \brief Partial template specialization for functions having one argument.
    /// \sa Function
    template<typename R, typename A0>
    struct Function<R(A0)> : public internal::Function0<R, A0>
    {
    };

    /// \brief Partial template specialization for functions having one argument.
    /// \sa Function
    template<typename R, typename A0>
    struct Function<R (*)(A0)> : public internal::Function0<R, A0>
    {
    };

    /// \brief Partial template specialization for functions having two arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1>
    struct Function<R(A0, A1)> : public internal::Function1<R, A0, A1>
    {
    };

    /// \brief Partial template specialization for functions having two arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1>
    struct Function<R (*)(A0, A1)> : public internal::Function1<R, A0, A1>
    {
    };

    /// \brief Partial template specialization for functions having three arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2>
    struct Function<R(A0, A1, A2)> : public internal::Function2<R, A0, A1, A2>
    {
    };

    /// \brief Partial template specialization for functions having three arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2>
    struct Function<R (*)(A0, A1, A2)> : public internal::Function2<R, A0, A1, A2>
    {
    };

    /// \brief Partial template specialization for functions having four arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2, typename A3>
    struct Function<R(A0, A1, A2, A3)> : public internal::Function3<R, A0, A1, A2, A3>
    {
    };

    /// \brief Partial template specialization for functions having four arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2, typename A3>
    struct Function<R (*)(A0, A1, A2, A3)> : public internal::Function3<R, A0, A1, A2, A3>
    {
    };

    /// \brief Partial template specialization for functions having five arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct Function<R(A0, A1, A2, A3, A4)> : public internal::Function4<R, A0, A1, A2, A3, A4>
    {
    };

    /// \brief Partial template specialization for functions having five arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct Function<R (*)(A0, A1, A2, A3, A4)> : public internal::Function4<R, A0, A1, A2, A3, A4>
    {
    };

    /// \brief Partial template specialization for functions having six arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
    struct Function<R(A0, A1, A2, A3, A4, A5)> : public internal::Function5<R, A0, A1, A2, A3, A4, A5>
    {
    };

    /// \brief Partial template specialization for functions having six arguments.
    /// \sa Function
    template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
    struct Function<R (*)(A0, A1, A2, A3, A4, A5)> : public internal::Function5<R, A0, A1, A2, A3, A4, A5>
    {
    };
} // namespace traits

X_NAMESPACE_END

#endif // X_FUNCTIONTRAITS_H_
