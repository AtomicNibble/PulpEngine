#pragma once


// Compiler

#define TELEM_ABSTRACT                              abstract
#define TELEM_OVERRIDE                              override
#define TELEM_FINAL                                 override final

#define TELEM_PRAGMA(pragma)                        __pragma(pragma)
#define TELEM_DISABLE_WARNING(number)               TELEM_PRAGMA(warning(disable: number))
#define TELEM_ENABLE_WARNING(number)                TELEM_PRAGMA(warning(default: number))
#define TELEM_PUSH_WARNING_LEVEL(level)             TELEM_PRAGMA(warning(push, level))
#define TELEM_POP_WARNING_LEVEL                     TELEM_PRAGMA(warning(pop))
#define TELEM_ALIGN_OF(type)                        __alignof(type)
#define TELEM_INLINE                                __forceinline
#define TELEM_NO_INLINE                             __declspec(noinline)
#define TELEM_HINT(hint)                            __assume(hint)
#define TELEM_NO_SWITCH_DEFAULT                     TELEM_HINT(0)
#define TELEM_RESTRICT                              __restrict
#define TELEM_RESTRICT_RV                           __declspec(restrict)
#define TELEM_NO_ALIAS                              __declspec(noalias)
#define TELEM_RETURN_ADDRESS()                      _ReturnAddress()
#define TELEM_LINK_LIB(libName)                     TELEM_PRAGMA(comment(lib, libName))
#define TELEM_ALIGNED_SYMBOL(symbol, alignment)     __declspec(align(alignment)) symbol
#define TELEM_ALIGN_OF(type)                        __alignof(type)
#define TELEM_OFFSETOF(s, m)                        offsetof(s, m)

#define TELEM_PACK_PUSH(val)                        TELEM_PRAGMA(pack(push, val))
#define TELEM_PACK_POP                              TELEM_PRAGMA(pack(pop))

#define TELEM_IMPORT                                __declspec(dllimport)
#define TELEM_EXPORT                                __declspec(dllexport)

#define TELEM_UNUSED(x)                             UNREFERENCED_PARAMETER(x)


#define TELEM_NO_CREATE(className) \
private:                       \
    className(void);           \
    ~className(void)
#define TELEM_NO_COPY(className) className(const className&) = delete
#define TELEM_NO_ASSIGN(className) className& operator=(const className&) = delete
#define TELEM_NO_ASSIGN_VOLATILE(className) className& operator=(const className&) volatile = delete

#define TELEM_NO_MOVE(className) className(className&&) = delete
#define TELEM_NO_MOVE_ASSIGN(className) className& operator=(className&&) = delete
