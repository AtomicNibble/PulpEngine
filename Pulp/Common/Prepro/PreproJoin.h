#pragma once

#ifndef X_PREPROCESSORJOIN_H
#define X_PREPROCESSORJOIN_H

#define X_PP_JOIN_HELPER_HELPER(_0, _1) _0##_1

#define X_PP_JOIN_HELPER(_0, _1) X_PP_JOIN_HELPER_HELPER(_0, _1)

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


#if X_COMPILER_CLANG
#define X_PP_JOIN(...) X_PP_JOIN_IMPL(X_PP_JOIN_, X_PP_VA_NUM_ARGS(__VA_ARGS__)) \
(__VA_ARGS__)
#else
#define X_PP_JOIN(...) X_PP_JOIN_IMPL(X_PP_JOIN_, X_PP_VA_NUM_ARGS(__VA_ARGS__)) \
X_PP_PASS_ARGS(__VA_ARGS__)
#endif // X_COMPILER_CLANG

#endif
