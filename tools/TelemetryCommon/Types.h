#pragma once

// define some types.
using tt_int8 = char;
using tt_int16 = short;
using tt_int32 = int;
using tt_int64 = long long;

using tt_uint8 = unsigned char;
using tt_uint16 = unsigned short;
using tt_uint32 = unsigned int;
using tt_uint64 = unsigned long long;

#ifdef X_64

using tt_intptr = __int64;
using tt_uintptr = unsigned __int64;
using tt_ptrdiff = __int64;
using tt_size = unsigned __int64;

#else

using tt_intptr = int;
using tt_uintptr = unsigned int;
using tt_ptrdiff = int;
using tt_size = unsigned int;

#endif

static_assert(sizeof(tt_uintptr) == sizeof(void*), "Size missmatch");