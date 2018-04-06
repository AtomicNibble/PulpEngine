#pragma once

#ifndef X_PREPROCESSORISEMPTY_H
#define X_PREPROCESSORISEMPTY_H

/// \def X_PP_IS_EMPTY_CASE_0001
/// \brief Internal macro used by \ref X_PP_IS_EMPTY.
/// \sa X_PP_IS_EMPTY
#define X_PP_IS_EMPTY_CASE_0001 ,

/// \def X_PP_IS_EMPTY_BRACKET_TEST
/// \brief Internal macro used by \ref X_PP_IS_EMPTY.
/// \sa X_PP_IS_EMPTY
#define X_PP_IS_EMPTY_BRACKET_TEST(...) ,

/// \def X_PP_IS_EMPTY
/// \ingroup Preprocessor
/// \brief Outputs 1 if the argument list passed to the variadic macro is empty, and 0 otherwise.
/// \details By combining the test result of four different cases, the macro can determine whether the argument list
/// is empty or not.
/// Depending on the outcome of the evaluation of the four cases, the macro that is being fed into the final evaluation
/// of \ref X_PP_HAS_COMMA is any of the following:
/// - X_PP_IS_EMPTY_CASE_0000
/// - X_PP_IS_EMPTY_CASE_0001
/// - X_PP_IS_EMPTY_CASE_0010
/// - X_PP_IS_EMPTY_CASE_0011
/// - X_PP_IS_EMPTY_CASE_0100
/// - X_PP_IS_EMPTY_CASE_0101
/// - ...
///
/// Of all these macros, only \ref X_PP_IS_EMPTY_CASE_0001 is actually defined. This means that if the resulting
/// evaluation is done with X_PP_HAS_COMMA(X_PP_IS_EMPTY_CASE_0001), this will expand to 1, meaning that the
/// argument list was indeed empty. For all other outcomes, e.g. X_PP_HAS_COMMA(X_PP_IS_EMPTY_CASE_1010), the
/// case macro will not be defined, resulting in an output of 0.
#define X_PP_IS_EMPTY(...)                                          \
    X_PP_HAS_COMMA(                                                 \
        X_PP_JOIN_5(                                                \
            X_PP_IS_EMPTY_CASE_,                                    \
            X_PP_HAS_COMMA(__VA_ARGS__),                            \
            X_PP_HAS_COMMA(X_PP_IS_EMPTY_BRACKET_TEST __VA_ARGS__), \
            X_PP_HAS_COMMA(__VA_ARGS__(~)),                         \
            X_PP_HAS_COMMA(X_PP_IS_EMPTY_BRACKET_TEST __VA_ARGS__(~))))

#endif
