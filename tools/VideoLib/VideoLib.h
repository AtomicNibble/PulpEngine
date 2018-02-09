#pragma once


#include <Core\Compiler.h>

#ifndef VIDEOLIB_EXPORT
#ifdef X_LIB
#define VIDEOLIB_EXPORT
#else
#ifdef VIDEO_LIB_EXPORT
#define VIDEOLIB_EXPORT X_EXPORT
#else
#define VIDEOLIB_EXPORT X_IMPORT
#endif // !VIDEO_LIB_EXPORT
#endif // X_LIB
#endif // !VIDEOLIB_EXPORT


