#pragma once

#ifndef X_PREPROCESSORLIST_H
#define X_PREPROCESSORLIST_H

#define X_PP_LIST_0(op)
#define X_PP_LIST_1(op) op(0)
#define X_PP_LIST_2(op) X_PP_LIST_1(op), op(1)
#define X_PP_LIST_3(op) X_PP_LIST_2(op), op(2)
#define X_PP_LIST_4(op) X_PP_LIST_3(op), op(3)
#define X_PP_LIST_5(op) X_PP_LIST_4(op), op(4)
#define X_PP_LIST_6(op) X_PP_LIST_5(op), op(5)
#define X_PP_LIST_7(op) X_PP_LIST_6(op), op(6)
#define X_PP_LIST_8(op) X_PP_LIST_7(op), op(7)
#define X_PP_LIST_9(op) X_PP_LIST_8(op), op(8)
#define X_PP_LIST_10(op) X_PP_LIST_9(op), op(9)
#define X_PP_LIST_11(op) X_PP_LIST_10(op), op(10)
#define X_PP_LIST_12(op) X_PP_LIST_11(op), op(11)
#define X_PP_LIST_13(op) X_PP_LIST_12(op), op(12)
#define X_PP_LIST_14(op) X_PP_LIST_13(op), op(13)
#define X_PP_LIST_15(op) X_PP_LIST_14(op), op(14)
#define X_PP_LIST_16(op) X_PP_LIST_15(op), op(15)
#define X_PP_LIST_17(op) X_PP_LIST_16(op), op(16)
#define X_PP_LIST_18(op) X_PP_LIST_17(op), op(17)
#define X_PP_LIST_19(op) X_PP_LIST_18(op), op(18)
#define X_PP_LIST_20(op) X_PP_LIST_19(op), op(19)
#define X_PP_LIST_21(op) X_PP_LIST_20(op), op(20)
#define X_PP_LIST_22(op) X_PP_LIST_21(op), op(21)
#define X_PP_LIST_23(op) X_PP_LIST_22(op), op(22)

/// \def X_PP_LIST
/// \ingroup Preprocessor
/// \brief Expands the arguments so that a user-defined operation is called N number of times.
/// \details This macro is useful for e.g. generating a list of arguments based on a predefined count.
///
/// The operation to call must be a macro with one parameter, which denotes how many times the operation has been called.
///
/// Check the provided code example for example usage:
/// \code
///   // assume that we want to define a function Invoke with different overloads for 0, 1, 2, etc. arguments
///   // define our operation macro
///   #define ARGS(n)					ARG##n arg##n
///
///   // assume COUNT has been defined to 0, 1, 2, etc.
///   void Invoke(X_PP_LIST(COUNT, ARGS_IMPL))
///
///   // based on the definition of COUNT, the output will be one of the following
///   void Invoke()
///   void Invoke(ARG0 arg0)
///   void Invoke(ARG0 arg0, ARG1 arg1)
///   void Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2)
///   ...
/// \endcode
#define X_PP_LIST(count, op) X_PP_JOIN_2(X_PP_LIST_, count) \
(op)

#endif
