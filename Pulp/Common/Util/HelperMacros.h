#pragma once

#ifndef _X_HELPER_MACROS_H_
#define _X_HELPER_MACROS_H_

#define X_VALIST_START(fmt) \
    va_list args;           \
    va_start(args, fmt);

#define X_VALIST_END \
    va_end(args);

#ifndef BIT
#define BIT(num) (1u << (num))
#endif

#endif // _X_HELPER_MACROS_H_