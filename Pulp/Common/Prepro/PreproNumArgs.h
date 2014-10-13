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
#	define X_PP_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...)	N
#	define X_PP_VA_NUM_ARGS_REVERSE_SEQUENCE			16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
#	define X_PP_VA_NUM_ARGS_LEFT (
#	define X_PP_VA_NUM_ARGS_RIGHT )
#	define X_PP_VA_NUM_ARGS(...)						X_PP_VA_NUM_ARGS_HELPER X_PP_VA_NUM_ARGS_LEFT __VA_ARGS__, X_PP_VA_NUM_ARGS_REVERSE_SEQUENCE X_PP_VA_NUM_ARGS_RIGHT
#else
#	define X_PP_VA_NUM_ARGS(...)						X_PP_VA_NUM_ARGS_HELPER(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#	define X_PP_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...)	N
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
#define X_PP_NUM_ARGS(...)								X_PP_IF(X_PP_IS_EMPTY(__VA_ARGS__), 0, X_PP_VA_NUM_ARGS(__VA_ARGS__))


#endif
