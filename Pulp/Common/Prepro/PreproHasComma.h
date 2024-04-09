#pragma once

#ifndef X_PREPROCESSORHASCOMMA_H_
#define X_PREPROCESSORHASCOMMA_H_

#define X_PP_HAS_COMMA_EVAL(...) __VA_ARGS__
#define X_PP_HAS_COMMA_ARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...) _16
#define X_PP_HAS_COMMA(...) X_PP_HAS_COMMA_EVAL(X_PP_HAS_COMMA_ARGS(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0))

#endif // X_PREPROCESSORHASCOMMA_H_
