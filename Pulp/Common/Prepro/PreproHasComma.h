#pragma once

#ifndef X_PREPROCESSORHASCOMMA_H_
#define X_PREPROCESSORHASCOMMA_H_

/// \def X_PP_HAS_COMMA_EVAL
/// \brief Internal macro used by \ref X_PP_HAS_COMMA.
/// \sa X_PP_HAS_COMMA
#define X_PP_HAS_COMMA_EVAL(...) __VA_ARGS__

/// \def X_PP_HAS_COMMA_ARGS
/// \brief Internal macro used by \ref X_PP_HAS_COMMA.
/// \sa X_PP_HAS_COMMA
#define X_PP_HAS_COMMA_ARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...) _16

/// \def X_PP_HAS_COMMA
/// \ingroup Preprocessor
/// \brief Outputs 1 if the arguments to the variadic macro are separated by a comma, and 0 otherwise.
/// \details This macro can be used for detecting if the given arguments are separated by a comma, and is internally
/// used for finding out whether the argument list to a variadic macro is empty or not. The latter needs to be known
/// when "counting" the number of arguments to a variadic macro.
/// \code
///   // the macro handles completely empty arguments
///   X_PP_HAS_COMMA();		// outputs 0
///   X_PP_HAS_COMMA(,);		// outputs 1
///   X_PP_HAS_COMMA(, ,);		// outputs 1
///
///   // the macro also handles cases of single and multi arguments
///   X_PP_HAS_COMMA(a);					// outputs 0
///   X_PP_HAS_COMMA((a, b));				// outputs 0, the comma only separates a and b!
///   X_PP_HAS_COMMA((a, b), (c, d));		// outputs 1, the comma separates (a, b) and (c, d)
/// \endcode
#define X_PP_HAS_COMMA(...) X_PP_HAS_COMMA_EVAL(X_PP_HAS_COMMA_ARGS(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0))

#endif // X_PREPROCESSORHASCOMMA_H_
