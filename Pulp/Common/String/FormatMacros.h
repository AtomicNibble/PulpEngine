#pragma once

#ifndef X_FORMAT_MACROS_H_
#define X_FORMAT_MACROS_H_

#if X_PLATFORM_WIN32

#include <inttypes.h>

#if !defined(PRIuS)
#define PRIuS "Iu"
#endif

#if !defined(PRIxS)
#define PRIxS "zx"
#endif

#define PRns "S"  // print a narrow string (when passed with a wide format string)
#define PRws "ls" // print a wide string (when passed with a narrow format string)

#else

#error "format macros not defined for this platform."

#endif

#endif // !X_FORMAT_MACROS_H_