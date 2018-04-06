#pragma once

#ifndef X_PREPROCESSORJOIN_H
#define X_PREPROCESSORJOIN_H

/// \def X_PP_JOIN_HELPER_HELPER
/// \brief Internal macro used by \ref X_PP_JOIN.
/// \sa X_PP_JOIN
#define X_PP_JOIN_HELPER_HELPER(_0, _1) _0##_1

/// \def X_PP_JOIN_HELPER
/// \brief Internal macro used by \ref X_PP_JOIN.
/// \sa X_PP_JOIN
#define X_PP_JOIN_HELPER(_0, _1) X_PP_JOIN_HELPER_HELPER(_0, _1)

/// \def X_PP_JOIN_IMPL
/// \brief Internal macro used by \ref X_PP_JOIN.
/// \sa X_PP_JOIN
#define X_PP_JOIN_IMPL(_0, _1) X_PP_JOIN_HELPER(_0, _1)

#define X_PP_JOIN_2(_0, _1) X_PP_JOIN_IMPL(_0, _1)
#define X_PP_JOIN_3(_0, _1, _2) X_PP_JOIN_2(X_PP_JOIN_2(_0, _1), _2)
#define X_PP_JOIN_4(_0, _1, _2, _3) X_PP_JOIN_2(X_PP_JOIN_3(_0, _1, _2), _3)
#define X_PP_JOIN_5(_0, _1, _2, _3, _4) X_PP_JOIN_2(X_PP_JOIN_4(_0, _1, _2, _3), _4)
#define X_PP_JOIN_6(_0, _1, _2, _3, _4, _5) X_PP_JOIN_2(X_PP_JOIN_5(_0, _1, _2, _3, _4), _5)
#define X_PP_JOIN_7(_0, _1, _2, _3, _4, _5, _6) X_PP_JOIN_2(X_PP_JOIN_6(_0, _1, _2, _3, _4, _5), _6)
#define X_PP_JOIN_8(_0, _1, _2, _3, _4, _5, _6, _7) X_PP_JOIN_2(X_PP_JOIN_7(_0, _1, _2, _3, _4, _5, _6), _7)
#define X_PP_JOIN_9(_0, _1, _2, _3, _4, _5, _6, _7, _8) X_PP_JOIN_2(X_PP_JOIN_8(_0, _1, _2, _3, _4, _5, _6, _7), _8)
#define X_PP_JOIN_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9) X_PP_JOIN_2(X_PP_JOIN_9(_0, _1, _2, _3, _4, _5, _6, _7, _8), _9)
#define X_PP_JOIN_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) X_PP_JOIN_2(X_PP_JOIN_10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9), _10)
#define X_PP_JOIN_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) X_PP_JOIN_2(X_PP_JOIN_11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10), _11)
#define X_PP_JOIN_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) X_PP_JOIN_2(X_PP_JOIN_12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11), _12)
#define X_PP_JOIN_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) X_PP_JOIN_2(X_PP_JOIN_13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12), _13)
#define X_PP_JOIN_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) X_PP_JOIN_2(X_PP_JOIN_14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13), _14)
#define X_PP_JOIN_16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) X_PP_JOIN_2(X_PP_JOIN_15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14), _15)

/// \def X_PP_JOIN
/// \ingroup Preprocessor
/// \brief Concatenates tokens, even when the tokens are macros themselves.
/// \details Because of the way the preprocessor works, macros can only be concatenated when using extra indirections.
/// This is being taken care of by the \ref X_PP_JOIN macro, and is mostly needed when dealing with macros where
/// one does not know whether it will be fed just simple tokens, or names of other macros as well.
/// Furthermore, because the preprocessor module has support for counting the number of arguments passed to a variadic
/// macro, the user does not need to call different macros based on the number of arguments given.
/// \code
///   #define MY_JOIN(a, b)				a##b
///   #define WORLD_NAME				world
///
///   // the token-pasting operator ## only works with simple tokens, not macros
///   MY_JOIN("hello", "world");			// outputs "hello""world"
///   MY_JOIN("hello", WORLD_NAME);			// outputs "hello"WORLD_NAME
///
///   X_PP_JOIN("hello", "world");			// outputs "hello""world"
///   X_PP_JOIN("hello", WORLD_NAME);		// outputs "hello""world"
///
///   // the macro is able to join any given number of arguments
///   X_PP_JOIN(a, b, c);					// outputs abc
///   X_PP_JOIN(a, b, c, d);				// outputs abcd
///   X_PP_JOIN(a, b, c, d, e);			// outputs abcde
/// \endcode
/// Note that no matter how many arguments we provide, the macro to use is always \ref X_PP_JOIN. This offers a powerful
/// facility for concatenating an unlimited amount of tokens.

#if X_COMPILER_CLANG
#define X_PP_JOIN(...) X_PP_JOIN_IMPL(X_PP_JOIN_, X_PP_VA_NUM_ARGS(__VA_ARGS__)) \
(__VA_ARGS__)
#else
#define X_PP_JOIN(...) X_PP_JOIN_IMPL(X_PP_JOIN_, X_PP_VA_NUM_ARGS(__VA_ARGS__)) \
X_PP_PASS_ARGS(__VA_ARGS__)
#endif // X_COMPILER_CLANG

#endif
