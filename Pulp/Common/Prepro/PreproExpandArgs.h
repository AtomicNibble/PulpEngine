#pragma once

#ifndef X_PREPROCESSOREXPANDARGS_H_
#define X_PREPROCESSOREXPANDARGS_H_


#define X_PP_EXPAND_ARGS_0(op, empty)
#define X_PP_EXPAND_ARGS_1(op, a1)																			op(a1, 0)
#define X_PP_EXPAND_ARGS_2(op, a1, a2)																		op(a1, 0) op(a2, 1)
#define X_PP_EXPAND_ARGS_3(op, a1, a2, a3)																	op(a1, 0) op(a2, 1) op(a3, 2)
#define X_PP_EXPAND_ARGS_4(op, a1, a2, a3, a4)																op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3)
#define X_PP_EXPAND_ARGS_5(op, a1, a2, a3, a4, a5)															op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4)
#define X_PP_EXPAND_ARGS_6(op, a1, a2, a3, a4, a5, a6)														op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5)
#define X_PP_EXPAND_ARGS_7(op, a1, a2, a3, a4, a5, a6, a7)													op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6)
#define X_PP_EXPAND_ARGS_8(op, a1, a2, a3, a4, a5, a6, a7, a8)												op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7)
#define X_PP_EXPAND_ARGS_9(op, a1, a2, a3, a4, a5, a6, a7, a8, a9)											op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8)
#define X_PP_EXPAND_ARGS_10(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)									op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9)
#define X_PP_EXPAND_ARGS_11(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)								op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10)
#define X_PP_EXPAND_ARGS_12(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)							op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10) op(a12, 11)
#define X_PP_EXPAND_ARGS_13(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)						op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10) op(a12, 11) op(a13, 12)
#define X_PP_EXPAND_ARGS_14(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)				op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10) op(a12, 11) op(a13, 12) op(a14, 13)
#define X_PP_EXPAND_ARGS_15(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)			op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10) op(a12, 11) op(a13, 12) op(a14, 13) op(a15, 14)
#define X_PP_EXPAND_ARGS_16(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)		op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10) op(a12, 11) op(a13, 12) op(a14, 13) op(a15, 14) op(a16, 15)
#define X_PP_EXPAND_ARGS_17(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17)				op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10) op(a12, 11) op(a13, 12) op(a14, 13) op(a15, 14) op(a16, 15) op(a17, 16)
#define X_PP_EXPAND_ARGS_18(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18)		op(a1, 0) op(a2, 1) op(a3, 2) op(a4, 3) op(a5, 4) op(a6, 5) op(a7, 6) op(a8, 7) op(a9, 8) op(a10, 9) op(a11, 10) op(a12, 11) op(a13, 12) op(a14, 13) op(a15, 14) op(a16, 15) op(a17, 16) op(a18, 17)


/// \def X_PP_EXPAND_ARGS
/// \ingroup Preprocessor
/// \brief Expands the arguments so that a user-defined operation is called with each argument separately.
/// \details This macro is useful for dispatching a variable number of arguments to any operation without having to
/// repeatedly call this operation. Because the preprocessor module has support for counting the number of arguments
/// passed to a variadic macro, the user does not need to call different macros based on the number of arguments given.
///
/// The operation to call must be a macro with two parameters:
/// - The first parameter is one of the arguments provided to the \ref X_PP_EXPAND_ARGS macro.
/// - The second parameter is the index of the argument in the variable argument list of the \ref X_PP_EXPAND_ARGS macro.
/// 
/// Check the provided code example for example usage:
/// \code
///   // assume that there exists a function called GlobalFunction(const char*, unsigned int, T)
///   // define our operation macro
///   #define CALL_FUNCTION(variable, n)			GlobalFunction(#variable, n, variable);
///
///   ...
///   int a = 10;
///   int b = 20;
///   X_PP_EXPAND_ARGS(CALL_FUNCTION, a, b);
///   will expand into
///   GlobalFunction("a", a); GlobalFunction("b", b);
///
///   ...
///   int a = 10;
///   int b = 20;
///   int c = 30;
///   X_PP_EXPAND_ARGS(CALL_FUNCTION, a, b, c);
///   will expand into
///   GlobalFunction("a", a); GlobalFunction("b", b); GlobalFunction("c", c);
/// \endcode
/// Note that no matter how many arguments we provide, the macro to use is always \ref X_PP_EXPAND_ARGS. Because
/// the provided \a op argument can itself be a macro, the \ref X_PP_EXPAND_ARGS macro offers a powerful facility for
/// e.g. chaining an unlimited amount of calls, as used by the assertion system.
#define X_PP_EXPAND_ARGS(op, ...)		X_PP_JOIN_2(X_PP_EXPAND_ARGS_, X_PP_NUM_ARGS(__VA_ARGS__)) X_PP_PASS_ARGS(op, __VA_ARGS__)


#endif // X_PREPROCESSOREXPANDARGS_H_
