#pragma once

#ifndef X_PREPROCESSORISEMPTY_H
#define X_PREPROCESSORISEMPTY_H

#define X_PP_IS_EMPTY_CASE_0001 ,

#define X_PP_IS_EMPTY_BRACKET_TEST(...) ,

#define X_PP_IS_EMPTY(...)                                          \
    X_PP_HAS_COMMA(                                                 \
        X_PP_JOIN_5(                                                \
            X_PP_IS_EMPTY_CASE_,                                    \
            X_PP_HAS_COMMA(__VA_ARGS__),                            \
            X_PP_HAS_COMMA(X_PP_IS_EMPTY_BRACKET_TEST __VA_ARGS__), \
            X_PP_HAS_COMMA(__VA_ARGS__(~)),                         \
            X_PP_HAS_COMMA(X_PP_IS_EMPTY_BRACKET_TEST __VA_ARGS__(~))))

#endif
