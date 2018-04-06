#pragma once

#ifndef X_PREPROCESSORTOBOOL_H
#define X_PREPROCESSORTOBOOL_H

#define X_PP_TO_BOOL_false 0
#define X_PP_TO_BOOL_true 1
#define X_PP_TO_BOOL_0 0
#define X_PP_TO_BOOL_1 1
#define X_PP_TO_BOOL_2 1
#define X_PP_TO_BOOL_3 1
#define X_PP_TO_BOOL_4 1
#define X_PP_TO_BOOL_5 1
#define X_PP_TO_BOOL_6 1
#define X_PP_TO_BOOL_7 1
#define X_PP_TO_BOOL_8 1
#define X_PP_TO_BOOL_9 1
#define X_PP_TO_BOOL_10 1
#define X_PP_TO_BOOL_11 1
#define X_PP_TO_BOOL_12 1
#define X_PP_TO_BOOL_13 1
#define X_PP_TO_BOOL_14 1
#define X_PP_TO_BOOL_15 1
#define X_PP_TO_BOOL_16 1

/// \def X_PP_TO_BOOL
/// \ingroup Preprocessor
/// \brief Converts a condition into either 1 if the condition is true, or 0 otherwise.
/// \details The condition can either be a boolean value, or an integer.
/// \code
///   // boolean values
///   X_PP_TO_BOOL(false)			outputs 0
///   X_PP_TO_BOOL(true)			outputs 1
///
///   // integer values
///   X_PP_TO_BOOL(0)				outputs 0
///   X_PP_TO_BOOL(1)				outputs 1
///   X_PP_TO_BOOL(2)				outputs 1
///   X_PP_TO_BOOL(3)				outputs 1
/// \endcode
#define X_PP_TO_BOOL(x) X_PP_JOIN_2(X_PP_TO_BOOL_, x)

#endif
