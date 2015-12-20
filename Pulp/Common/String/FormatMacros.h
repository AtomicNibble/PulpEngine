#pragma once


#ifndef X_FORMAT_MACROS_H_
#define X_FORMAT_MACROS_H_


#if X_PLATFORM_WIN32

#include <inttypes.h>

#if !defined(PRIuS)
#define PRIuS "Iu"
#endif

#else

#error "format macros not defined for this platform."

#endif

#endif // !X_FORMAT_MACROS_H_