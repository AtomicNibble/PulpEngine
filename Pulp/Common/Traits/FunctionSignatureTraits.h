#pragma once

#ifndef X_FUNCTIONSIGNATURETRAITS_H_
#define X_FUNCTIONSIGNATURETRAITS_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Traits
namespace traits
{
    /// \ingroup Traits
    namespace internal
    {
        /// Internal struct used by \ref traits::Function and \ref traits::MemberFunction.
        template<typename R>
        struct FunctionSignature
        {
            typedef R ReturnType;
        };

        /// Internal struct used by \ref traits::Function and \ref traits::MemberFunction.
        template<typename R, typename A0>
        struct FunctionSignature0 : public FunctionSignature<R>
        {
            typedef A0 Arg0;
        };

        /// Internal struct used by \ref traits::Function and \ref traits::MemberFunction.
        template<typename R, typename A0, typename A1>
        struct FunctionSignature1 : public FunctionSignature0<R, A0>
        {
            typedef A1 Arg1;
        };

        /// Internal struct used by \ref traits::Function and \ref traits::MemberFunction.
        template<typename R, typename A0, typename A1, typename A2>
        struct FunctionSignature2 : public FunctionSignature1<R, A0, A1>
        {
            typedef A2 Arg2;
        };

        /// Internal struct used by \ref traits::Function and \ref traits::MemberFunction.
        template<typename R, typename A0, typename A1, typename A2, typename A3>
        struct FunctionSignature3 : public FunctionSignature2<R, A0, A1, A2>
        {
            typedef A3 Arg3;
        };

        /// Internal struct used by \ref traits::Function and \ref traits::MemberFunction.
        template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4>
        struct FunctionSignature4 : public FunctionSignature3<R, A0, A1, A2, A3>
        {
            typedef A4 Arg4;
        };

        /// Internal struct used by \ref traits::Function and \ref traits::MemberFunction.
        template<typename R, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
        struct FunctionSignature5 : public FunctionSignature4<R, A0, A1, A2, A3, A4>
        {
            typedef A5 Arg5;
        };
    } // namespace internal
} // namespace traits

X_NAMESPACE_END

#endif // X_FUNCTIONSIGNATURETRAITS_H_
