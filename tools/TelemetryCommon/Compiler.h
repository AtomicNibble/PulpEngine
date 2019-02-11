#pragma once


// Compiler

#define X_ABSTRACT                              abstract
#define X_OVERRIDE                              override
#define X_FINAL                                 override final

#define X_PRAGMA(pragma)                        __pragma(pragma)
#define X_DISABLE_WARNING(number)               X_PRAGMA(warning(disable: number))
#define X_ENABLE_WARNING(number)                X_PRAGMA(warning(default: number))
#define X_PUSH_WARNING_LEVEL(level)             X_PRAGMA(warning(push, level))
#define X_POP_WARNING_LEVEL                     X_PRAGMA(warning(pop))
#define X_ALIGN_OF(type)                        __alignof(type)
#define X_INLINE                                __forceinline
#define X_NO_INLINE                             __declspec(noinline)
#define X_HINT(hint)                            __assume(hint)
#define X_NO_SWITCH_DEFAULT                     X_HINT(0)
#define X_RESTRICT                              __restrict
#define X_RESTRICT_RV                           __declspec(restrict)
#define X_NO_ALIAS                              __declspec(noalias)
#define X_RETURN_ADDRESS()                      _ReturnAddress()
#define X_LINK_LIB(libName)                     X_PRAGMA(comment(lib, libName))
#define X_ALIGNED_SYMBOL(symbol, alignment)     __declspec(align(alignment)) symbol
#define X_ALIGN_OF(type)                        __alignof(type)
#define X_OFFSETOF(s, m)                        offsetof(s, m)

#define X_PACK_PUSH(val)                        X_PRAGMA(pack(push, val))
#define X_PACK_POP                              X_PRAGMA(pack(pop))

#define X_IMPORT                                __declspec(dllimport)
#define X_EXPORT                                __declspec(dllexport)


#define X_UNUSED(x)                             UNREFERENCED_PARAMETER(x)