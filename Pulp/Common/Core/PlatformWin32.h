#pragma once

#ifndef _X_PLATFORM_WIN32_H_
#define _X_PLATFORM_WIN32_H_

#include "Core\Compiler.h"

#if X_64 == 0
#ifndef _CPU_X86
#define _CPU_X86
#endif
#ifndef _CPU_SSE
#define _CPU_SSE
#endif
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // _WIN32_WINNT_WIN7
#endif

// enable strict type checking for Windows headers
#ifndef STRICT
#define STRICT
#endif

// exclude rarely used Windows stuff
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define VC_EXTRALEAN

// We don't want all your shit!
#define NOGDICAPMASKS
// #define NOMENUS			// add for custom frame
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOATOM
// #define NODRAWTEXT		// add for custom frame
#define NOKERNEL
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NOSERVICE
#define NOSOUND
#define NOWINDOWSTATION
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

// suck my d(nipples)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

X_PUSH_WARNING_LEVEL(0)
#include <Windows.h>
// if not included here, C4548 warnings will be triggered elsewhere (expression before comma has no effect)
#include <malloc.h>
X_POP_WARNING_LEVEL

// undefine global namespace pollution
#ifdef CopyFile
#undef CopyFile
#endif

#ifdef FindText
#undef FindText
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef MAX_PRIORITY
#undef MAX_PRIORITY
#endif

#ifdef DELETE
#undef DELETE
#endif

#if defined(ERROR)
#undef ERROR
#endif

#ifdef Yield
#undef Yield
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef GetCurrentDirectory
#undef GetCurrentDirectory
#endif

#ifdef X_LIB
#define DLL_EXPORT
#define DLL_IMPORT
#else // !X_LIB
#define DLL_EXPORT X_EXPORT
#define DLL_IMPORT X_IMPORT
#endif // !X_LIB

#endif // !_X_PLATFORM_WIN32_H_
