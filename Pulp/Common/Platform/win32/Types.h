#pragma once

#ifndef _X_TYPES_H_
#define _X_TYPES_H_

#ifdef DWORD
#undef DWORD
#endif

#ifdef LONG
#undef LONG
#endif

#ifdef WORD
#undef WORD
#endif

#ifdef BYTE
#undef BYTE
#endif

#ifdef FLOAT
#undef FLOAT
#endif

#ifdef VOID
#undef VOID
#endif

#ifdef BOOL
#undef BOOL
#endif

#ifdef INT
#undef INT
#endif

#ifdef UINT
#undef UINT
#endif

typedef HWND PLATFORM_HWND;

typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef float FLOAT;
typedef void VOID;

typedef int BOOL;
typedef int INT;
typedef unsigned int UINT;

typedef unsigned __int64 QWORD;

// kinky !

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int_least8_t;
typedef short int_least16_t;
typedef int int_least32_t;

typedef unsigned char uint_least8_t;
typedef unsigned short uint_least16_t;
typedef unsigned int uint_least32_t;

//typedef char int_fast8_t;
//typedef int int_fast16_t;
//typedef int int_fast32_t;

typedef unsigned char uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned int uint_fast32_t;

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef uint32 uint;
typedef signed int int32;

typedef unsigned __int64 uint64;
typedef signed __int64 int64;

// typedef int INT_PTR, *PINT_PTR;
// typedef unsigned int UINT_PTR, *PUINT_PTR;

/* VC++ COMPILER PARAMETERS */
#if _MSC_FULL_VER >= 190023506
#include <stdint.h>
#else

#endif // !_MSC_FULL_VER < 190023506

#ifndef _PTRDIFF_T_DEFINED
#ifdef _WIN64
typedef __int64 ptrdiff_t;
#else  /* _WIN64 */
typedef int ptrdiff_t;
#endif /* _WIN64 */
#define _PTRDIFF_T_DEFINED
#endif /* _PTRDIFF_T_DEFINED */

#undef INT8_MIN
#undef INT8_MAX
#undef INT16_MIN
#undef INT16_MAX
#undef INT32_MIN
#undef INT32_MAX
#undef INT64_MIN
#undef INT64_MAX

#undef UINT8_MAX
#undef UINT16_MAX
#undef UINT32_MAX
#undef UINT64_MAX

#undef F32NAN
#undef F64NAN

const uint8 UINT8_MIN = 0;
const uint8 UINT8_MAX = 0xFFU;
const uint16 UINT16_MIN = 0;
const uint16 UINT16_MAX = 0xFFFFU;
const uint32 UINT32_MIN = 0;
const uint32 UINT32_MAX = 0xFFFFFFFFU;
const uint64 UINT64_MIN = 0;
const uint64 UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL; //0xFFFFFFFFFFFFFFFFui64;

const int8 INT8_MIN = -128;
const int8 INT8_MAX = 127;
const int16 INT16_MIN = -32768;
const int16 INT16_MAX = 32767;
const int32 INT32_MIN = (-2147483647 - 1);
const int32 INT32_MAX = 2147483647;
const int64 INT64_MIN = (int64)0x8000000000000000ULL; //(-9223372036854775807i64 - 1);
const int64 INT64_MAX = (int64)0x7FFFFFFFFFFFFFFFULL; // 9223372036854775807i64;

const int32 FLOAT_32NAN = (int32)0x7F800001;
const int64 FLOAT_64NAN = (int64)0x7FF0000000000001;

typedef float float32_t, f32;
typedef double float64_t, f64;

static_assert(sizeof(float32_t) == 4, "sizeof(float32_t) is not 4 bytes");
static_assert(sizeof(float64_t) == 8, "sizeof(float64_t) is not 8 bytes");

static_assert(sizeof(uint8_t) == 1, "sizeof(uint8_t) is not 1 byte");
static_assert(sizeof(int8_t) == 1, "sizeof(int8_t) is not 1 byte");

static_assert(sizeof(uint16_t) == 2, "sizeof(uint16_t) is not 2 bytes");
static_assert(sizeof(int16_t) == 2, "sizeof(int16_t) is not 2 bytes");

static_assert(sizeof(uint32_t) == 4, "sizeof(uint32_t) is not 4 bytes");
static_assert(sizeof(int32_t) == 4, "sizeof(int32_t) is not 4 bytes");

static_assert(sizeof(uint64_t) == 8, "sizeof(uint64_t) is not 8 bytes");
static_assert(sizeof(int64_t) == 8, "sizeof(int64_t) is not 8 bytes");

static_assert(sizeof(QWORD) == 8, "sizeof(QWORD) is not 8 bytes");

#endif // !_X_TYPES_H_
