#pragma once

#ifndef X_MEMBERFUNCTIONTRAITS_H_
#define X_MEMBERFUNCTIONTRAITS_H_

#include "Traits/FunctionSignatureTraits.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup Traits
namespace traits
{
    /// \ingroup Traits
    namespace internal
    {
        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R>
        struct MemberFunction : public FunctionSignature<R>
        {
            typedef R (C::*Pointer)(void);
            typedef R (C::*ConstPointer)(void) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0>
        struct MemberFunction0 : public FunctionSignature0<R, A0>
        {
            typedef R (C::*Pointer)(A0);
            typedef R (C::*ConstPointer)(A0) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1>
        struct MemberFunction1 : public FunctionSignature1<R, A0, A1>
        {
            typedef R (C::*Pointer)(A0, A1);
            typedef R (C::*ConstPointer)(A0, A1) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2>
        struct MemberFunction2 : public FunctionSignature2<R, A0, A1, A2>
        {
            typedef R (C::*Pointer)(A0, A1, A2);
            typedef R (C::*ConstPointer)(A0, A1, A2) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2, typename A3>
        struct MemberFunction3 : public FunctionSignature3<R, A0, A1, A2, A3>
        {
            typedef R (C::*Pointer)(A0, A1, A2, A3);
            typedef R (C::*ConstPointer)(A0, A1, A2, A3) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
        struct MemberFunction4 : public FunctionSignature4<R, A0, A1, A2, A3, A4>
        {
            typedef R (C::*Pointer)(A0, A1, A2, A3, A4);
            typedef R (C::*ConstPointer)(A0, A1, A2, A3, A4) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
        struct MemberFunction5 : public FunctionSignature5<R, A0, A1, A2, A3, A4, A5>
        {
            typedef R (C::*Pointer)(A0, A1, A2, A3, A4, A5);
            typedef R (C::*ConstPointer)(A0, A1, A2, A3, A4, A5) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R>
        struct MemberFunctionStd : public FunctionSignature<R>
        {
            typedef R (__stdcall C::*Pointer)(void);
            typedef R (__stdcall C::*ConstPointer)(void) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0>
        struct MemberFunctionStd0 : public FunctionSignature0<R, A0>
        {
            typedef R (__stdcall C::*Pointer)(A0);
            typedef R (__stdcall C::*ConstPointer)(A0) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1>
        struct MemberFunctionStd1 : public FunctionSignature1<R, A0, A1>
        {
            typedef R (__stdcall C::*Pointer)(A0, A1);
            typedef R (__stdcall C::*ConstPointer)(A0, A1) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2>
        struct MemberFunctionStd2 : public FunctionSignature2<R, A0, A1, A2>
        {
            typedef R (__stdcall C::*Pointer)(A0, A1, A2);
            typedef R (__stdcall C::*ConstPointer)(A0, A1, A2) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2, typename A3>
        struct MemberFunctionStd3 : public FunctionSignature3<R, A0, A1, A2, A3>
        {
            typedef R (__stdcall C::*Pointer)(A0, A1, A2, A3);
            typedef R (__stdcall C::*ConstPointer)(A0, A1, A2, A3) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
        struct MemberFunctionStd4 : public FunctionSignature4<R, A0, A1, A2, A3, A4>
        {
            typedef R (__stdcall C::*Pointer)(A0, A1, A2, A3, A4);
            typedef R (__stdcall C::*ConstPointer)(A0, A1, A2, A3, A4) const;
        };

        /// Internal struct used by \ref traits::MemberFunction.
        template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
        struct MemberFunctionStd5 : public FunctionSignature5<R, A0, A1, A2, A3, A4, A5>
        {
            typedef R (__stdcall C::*Pointer)(A0, A1, A2, A3, A4, A5);
            typedef R (__stdcall C::*ConstPointer)(A0, A1, A2, A3, A4, A5) const;
        };
    } // namespace internal

    /// \ingroup Traits
    /// \brief A traits class for member functions with arbitrary signature.
    /// \details This class template helps with declaring pointers-to-member-functions in a meaningful and descriptive
    /// way by providing template specializations and a corresponding typedef for member functions with arbitrary signature
    /// (up to 6 arguments).
    ///
    /// Using the provided template classes, declaring pointers-to-member-functions is easy, as shown in the following example:
    /// \code
    ///   // declares a member function of Class taking an int and returning nothing
    ///   typedef core::traits::MemberFunction<Class, void (int)> MyMethod;
    ///
    ///   // declares a pointer-to-member-function of Class taking an int and returning nothing
    ///   typedef MyMethod::Pointer PointerToMyMethod;
    /// \endcode
    /// \code
    ///   // declares a member function of Class taking a char and an int, returning a bool
    ///   typedef core::traits::MemberFunction<Class, bool (char, int)> MyMethod;
    ///
    ///   // declares a pointer-to-const-member-function of Class taking a char and an int, returning a bool
    ///   typedef MyMethod::ConstPointer PointerToConstMyMethod;
    /// \endcode
    /// Furthermore, the class template helps in declaring return values and argument types from deduced
    /// template arguments. Each function template defines the following nested types:
    /// - ReturnType: the type of the return value
    /// - Arg0-Arg5: the types of the arguments (up to 6 are supported)
    ///
    /// This feature is especially useful when forwarding arguments to other functions, e.g. when interfacing with
    /// scripting languages. See \ref traits::Function for an example.
    /// \sa Function
    template<class C, typename T>
    struct MemberFunction
    {
    };

    /// \brief Partial template specialization for member functions having no arguments.
    /// \sa MemberFunction
    template<class C, typename R>
    struct MemberFunction<C, R(void)> : public internal::MemberFunction<C, R>
    {
    };

    /// \brief Partial template specialization for member functions having no arguments.
    /// \sa MemberFunction
    template<class C, typename R>
    struct MemberFunction<C, R (C::*)(void)> : public internal::MemberFunction<C, R>
    {
    };

    /// \brief Partial template specialization for member functions having one argument.
    /// \sa MemberFunction
    template<class C, typename R, typename A0>
    struct MemberFunction<C, R(A0)> : public internal::MemberFunction0<C, R, A0>
    {
    };

    /// \brief Partial template specialization for member functions having one argument.
    /// \sa MemberFunction
    template<class C, typename R, typename A0>
    struct MemberFunction<C, R (C::*)(A0)> : public internal::MemberFunction0<C, R, A0>
    {
    };

    /// \brief Partial template specialization for member functions having two arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1>
    struct MemberFunction<C, R(A0, A1)> : public internal::MemberFunction1<C, R, A0, A1>
    {
    };

    /// \brief Partial template specialization for member functions having two arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1>
    struct MemberFunction<C, R (C::*)(A0, A1)> : public internal::MemberFunction1<C, R, A0, A1>
    {
    };

    /// \brief Partial template specialization for member functions having three arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2>
    struct MemberFunction<C, R(A0, A1, A2)> : public internal::MemberFunction2<C, R, A0, A1, A2>
    {
    };

    /// \brief Partial template specialization for member functions having three arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2>
    struct MemberFunction<C, R (C::*)(A0, A1, A2)> : public internal::MemberFunction2<C, R, A0, A1, A2>
    {
    };

    /// \brief Partial template specialization for member functions having four arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2, typename A3>
    struct MemberFunction<C, R(A0, A1, A2, A3)> : public internal::MemberFunction3<C, R, A0, A1, A2, A3>
    {
    };

    /// \brief Partial template specialization for member functions having four arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2, typename A3>
    struct MemberFunction<C, R (C::*)(A0, A1, A2, A3)> : public internal::MemberFunction3<C, R, A0, A1, A2, A3>
    {
    };

    /// \brief Partial template specialization for member functions having five arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct MemberFunction<C, R(A0, A1, A2, A3, A4)> : public internal::MemberFunction4<C, R, A0, A1, A2, A3, A4>
    {
    };

    /// \brief Partial template specialization for member functions having five arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct MemberFunction<C, R (C::*)(A0, A1, A2, A3, A4)> : public internal::MemberFunction4<C, R, A0, A1, A2, A3, A4>
    {
    };

    /// \brief Partial template specialization for member functions having six arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
    struct MemberFunction<C, R(A0, A1, A2, A3, A4, A5)> : public internal::MemberFunction5<C, R, A0, A1, A2, A3, A4, A5>
    {
    };

    /// \brief Partial template specialization for member functions having six arguments.
    /// \sa MemberFunction
    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
    struct MemberFunction<C, R (C::*)(A0, A1, A2, A3, A4, A5)> : public internal::MemberFunction5<C, R, A0, A1, A2, A3, A4, A5>
    {
    };

    // Explicit STD convention
    template<class C, typename T>
    struct MemberFunctionStd
    {
    };

    template<class C, typename R>
    struct MemberFunctionStd<C, R(void)> : public internal::MemberFunctionStd<C, R>
    {
    };

    template<class C, typename R>
    struct MemberFunctionStd<C, R (C::*)(void)> : public internal::MemberFunctionStd<C, R>
    {
    };

    template<class C, typename R, typename A0>
    struct MemberFunctionStd<C, R(A0)> : public internal::MemberFunctionStd0<C, R, A0>
    {
    };

    template<class C, typename R, typename A0>
    struct MemberFunctionStd<C, R (C::*)(A0)> : public internal::MemberFunctionStd0<C, R, A0>
    {
    };

    template<class C, typename R, typename A0, typename A1>
    struct MemberFunctionStd<C, R(A0, A1)> : public internal::MemberFunctionStd1<C, R, A0, A1>
    {
    };

    template<class C, typename R, typename A0, typename A1>
    struct MemberFunctionStd<C, R (C::*)(A0, A1)> : public internal::MemberFunctionStd1<C, R, A0, A1>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2>
    struct MemberFunctionStd<C, R(A0, A1, A2)> : public internal::MemberFunctionStd2<C, R, A0, A1, A2>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2>
    struct MemberFunctionStd<C, R (C::*)(A0, A1, A2)> : public internal::MemberFunctionStd2<C, R, A0, A1, A2>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2, typename A3>
    struct MemberFunctionStd<C, R(A0, A1, A2, A3)> : public internal::MemberFunctionStd3<C, R, A0, A1, A2, A3>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2, typename A3>
    struct MemberFunctionStd<C, R (C::*)(A0, A1, A2, A3)> : public internal::MemberFunctionStd3<C, R, A0, A1, A2, A3>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct MemberFunctionStd<C, R(A0, A1, A2, A3, A4)> : public internal::MemberFunctionStd4<C, R, A0, A1, A2, A3, A4>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
    struct MemberFunctionStd<C, R (C::*)(A0, A1, A2, A3, A4)> : public internal::MemberFunctionStd4<C, R, A0, A1, A2, A3, A4>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
    struct MemberFunctionStd<C, R(A0, A1, A2, A3, A4, A5)> : public internal::MemberFunctionStd5<C, R, A0, A1, A2, A3, A4, A5>
    {
    };

    template<class C, typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
    struct MemberFunctionStd<C, R (C::*)(A0, A1, A2, A3, A4, A5)> : public internal::MemberFunctionStd5<C, R, A0, A1, A2, A3, A4, A5>
    {
    };

} // namespace traits

X_NAMESPACE_END

#endif // X_MEMBERFUNCTIONTRAITS_H_
