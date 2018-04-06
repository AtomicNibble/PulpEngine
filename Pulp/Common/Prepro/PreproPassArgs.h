#pragma once

#ifndef X_PREPROCESSORPASSARGS_H
#define X_PREPROCESSORPASSARGS_H

/// \def X_PP_PASS_ARGS
/// \ingroup Preprocessor
/// \brief Passes <tt>__VA_ARGS__</tt> and other arguments as multiple parameters to another macro.
/// \details Because of a visual studio preprocessor bug which treats a <tt>__VA_ARGS__</tt> argument as being one single parameter
/// when passed to other macros, we need an extra macro which works around this bug.
/// See https://connect.microsoft.com/VisualStudio/feedback/details/521844/variadic-macro-treating-va-args-as-a-single-parameter-for-other-macros#details
/// for details.
/// \sa X_PP_NUM_ARGS
#if X_COMPILER_CLANG

#else

#if _MSC_VER >= 1400
#define X_PP_PASS_ARGS_LEFT (
#define X_PP_PASS_ARGS_RIGHT )
#define X_PP_PASS_ARGS(...) X_PP_PASS_ARGS_LEFT __VA_ARGS__ X_PP_PASS_ARGS_RIGHT
#else
#define X_PP_PASS_ARGS(...) (__VA_ARGS__)
#endif

#endif // X_COMPILER_CLANG

#endif
