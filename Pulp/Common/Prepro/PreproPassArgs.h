#pragma once

#ifndef X_PREPROCESSORPASSARGS_H
#define X_PREPROCESSORPASSARGS_H

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
