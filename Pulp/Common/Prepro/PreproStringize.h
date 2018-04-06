#pragma once

#ifndef X_PREPROCESSORSTRINGIZE_H
#define X_PREPROCESSORSTRINGIZE_H

/// \def X_PP_STRINGIZE_HELPER
/// \brief Internal macro used by \ref X_PP_STRINGIZE.
/// \sa X_PP_STRINGIZE
#define X_PP_STRINGIZE_HELPER(token) #token

/// \def X_PP_STRINGIZE
/// \ingroup Preprocessor
/// \brief Stringizes a given token, even when the given token is a macro itself.
/// \details Because of the way the preprocessor works, macros can only be stringized when using an extra indirection.
/// This is being taken care of by the \ref X_PP_STRINGIZE macro, and is mostly needed when dealing with macros where
/// one does not know whether it will be fed just simple tokens, or names of other macros as well. In addition,
/// the built-in stringizing operator does not work with predefined preprocessor symbols such as <tt>__LINE__</tt>.
/// \code
///   #define MY_MACRO(token)			#token
///   #define WORLD_NAME				world
///
///   // the stringizing operator # only works with simple tokens, not macros
///   MY_MACRO(hello);					// outputs "hello"
///   MY_MACRO(WORLD_NAME);				// outputs "WORLD_NAME"
///
///   X_PP_STRINGIZE(hello);			// outputs "hello"
///   X_PP_STRINGIZE(WORLD_NAME);		// outputs "world"
/// \endcode
#define X_PP_STRINGIZE(str) X_PP_STRINGIZE_HELPER(str)

#endif
