#pragma once

#ifndef X_PREPROCESSORNUMARGS_H_
#define X_PREPROCESSORNUMARGS_H_

/// \def X_PP_VA_NUM_ARGS
/// \brief Internal macro used by \ref X_PP_NUM_ARGS.
/// \details This macro is able to count the number of arguments for 1 to N provided arguments. In the case of completely
/// empty arguments, it will wrongly output 1. This is known and there is no workaround - instead, users should always
/// use the \ref X_PP_NUM_ARGS macro which handles all cases correctly. \ref X_PP_NUM_ARGS exists only to increase
/// source code readability, and is only used internally.
/// \remark The visual studio preprocessor has a bug which treats a <tt>__VA_ARGS__</tt> argument as being one single parameter
/// when passed to other macros.
/// See https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
/// for details. \ref X_PP_VA_NUM_ARGS contains a workaround for this bug.
/// \sa X_PP_NUM_ARGS
#if _MSC_VER >= 1400
#define X_PP_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, N, ...) N
#define X_PP_VA_NUM_ARGS_REVERSE_SEQUENCE 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#define X_PP_VA_NUM_ARGS_LEFT (
#define X_PP_VA_NUM_ARGS_RIGHT )
#define X_PP_VA_NUM_ARGS(...) X_PP_VA_NUM_ARGS_HELPER X_PP_VA_NUM_ARGS_LEFT __VA_ARGS__, X_PP_VA_NUM_ARGS_REVERSE_SEQUENCE X_PP_VA_NUM_ARGS_RIGHT
#else
#define X_PP_VA_NUM_ARGS(...) X_PP_VA_NUM_ARGS_HELPER(__VA_ARGS__, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define X_PP_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, N, ...) N
#endif

/// \def X_PP_NUM_ARGS
/// \ingroup Preprocessor
/// \brief Outputs the number of arguments passed to a variadic macro.
/// \details For certain preprocessor operations such as automatic code generation, it is helpful to have a facility
/// which is able to count the number of arguments passed to a variadic macro, which in turn makes it possible to have
/// macro "overloads" with different numbers of arguments.
/// \code
///   X_PP_NUM_ARGS();				// outputs 0
///   X_PP_NUM_ARGS(a);			// outputs 1
///   X_PP_NUM_ARGS(a, b);			// outputs 2
///   X_PP_NUM_ARGS(a, b, c);		// outputs 3
/// \endcode
/// \remark The macro relies on \ref X_PP_VA_NUM_ARGS to count the number of arguments for 1 to N arguments. Using
/// \ref X_PP_IF and \ref X_PP_IS_EMPTY it also handles the edge case of zero arguments, which cannot be done in a
/// generic way.

#if X_COMPILER_CLANG && 0
#define X_PP_NUM_ARGS(...) X_PP_VA_NUM_ARGS(__VA_ARGS__)
#else
#define X_PP_NUM_ARGS(...) X_PP_IF(X_PP_IS_EMPTY(__VA_ARGS__), 0, X_PP_VA_NUM_ARGS(__VA_ARGS__))
#endif

#endif
