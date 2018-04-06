#pragma once

#ifndef X_PREPROCESSORCOMMAIF_H_
#define X_PREPROCESSORCOMMAIF_H_

/// \def X_PP_COMMA
/// \brief Internal macro used by \ref X_PP_COMMA_IF.
/// \sa X_PP_COMMA_IF
#define X_PP_COMMA ,

/// \def X_PP_COMMA_EMPTY
/// \brief Internal macro used by \ref X_PP_COMMA_IF.
/// \sa X_PP_COMMA_IF
#define X_PP_COMMA_EMPTY

/// \def X_PP_COMMA_IF
/// \ingroup Preprocessor
/// \brief Outputs a comma if the given condition is true (!= 0), otherwise outputs nothing.
/// \details This macro can be used to output a comma for a comma-separated list of arguments based on some condition,
/// such as e.g. the number of arguments for any declaration. The following example shows a common usage:
/// \code
///   // assuming ARG_TYPENAMES is a macro, it can either be a comma-separated list of arguments, or empty
///   template <typename R, ARG_TYPENAMES>
///
///   // if ARG_TYPENAMES contains a comma-separated list of arguments, the macro expands to the following
///   template <typename R, typename T1, typename T2, ...>
///
///   // if ARG_TYPENAMES is empty, the extra comma would cause a syntax error
///   template <typename R, >
///
///   // using X_PP_COMMA_IF corrects this problem without having to special-case each such macro.
///   // this will only output a comma if COUNT != 0, solving the problem.
///   template <typename R X_PP_COMMA_IF(COUNT) ARG_TYPENAMES>
/// \endcode
#define X_PP_COMMA_IF(cond) X_PP_IF(cond, X_PP_COMMA, X_PP_COMMA_EMPTY)

#endif // X_PREPROCESSORCOMMAIF_H_
