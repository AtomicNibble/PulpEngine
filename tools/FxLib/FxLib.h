#pragma once


#include <Core\Compiler.h>

#ifndef FxLib_EXPORT
#ifdef X_LIB
#define FxLib_EXPORT
#else
#ifdef VIDEO_LIB_EXPORT
#define FxLib_EXPORT X_EXPORT
#else
#define FxLib_EXPORT X_IMPORT
#endif // !VIDEO_LIB_EXPORT
#endif // X_LIB
#endif // !FxLib_EXPORT


