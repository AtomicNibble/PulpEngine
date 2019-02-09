#pragma once

// Compiler

#define X_ABSTRACT                              abstract
#define X_OVERRIDE                              override
#define X_FINAL                                 override final

#define X_PRAGMA(pragma)                        __pragma(pragma)
#define X_PUSH_WARNING_LEVEL(level)             X_PRAGMA(warning(push, level))
#define X_POP_WARNING_LEVEL                     X_PRAGMA(warning(pop))
#define X_ALIGN_OF(type)                        __alignof(type)
#define X_INLINE                                __forceinline
#define X_NO_INLINE                             __declspec(noinline)
#define X_HINT(hint)                            __assume(hint)
#define X_RESTRICT                              __restrict
#define X_RESTRICT_RV                           __declspec(restrict)
#define X_NO_ALIAS                              __declspec(noalias)
#define X_RETURN_ADDRESS()                      _ReturnAddress()
#define X_LINK_LIB(libName)                     X_PRAGMA(comment(lib, libName))
#define X_ALIGNED_SYMBOL(symbol, alignment)     __declspec(align(alignment)) symbol
#define X_ALIGN_OF(type)                        __alignof(type)

#define X_PACK_PUSH(val)                        X_PRAGMA(pack(push, val))
#define X_PACK_POP                              X_PRAGMA(pack(pop))

#define X_IMPORT                                __declspec(dllimport)
#define X_EXPORT                                __declspec(dllexport)


#define X_UNUSED(x)                             UNREFERENCED_PARAMETER(x)


// -----------------------------------------------------------------


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


#include <cstdio>

namespace platform
{
#ifndef NEAR
#define NEAR
#endif

#ifndef FAR
#define FAR
#endif

#include <WinSock2.h>
#include <Ws2tcpip.h>

X_LINK_LIB("Ws2_32.lib");

} // namespace platform
