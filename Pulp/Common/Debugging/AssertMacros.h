#pragma once

#ifndef X_ASSERTMACROS_H_
#define X_ASSERTMACROS_H_

#include "Casts/union_cast.h"

X_NAMESPACE_BEGIN(core)

namespace internal
{
    template<typename T>
    inline bool IsAligned(T value, unsigned int alignment, unsigned int offset)
    {
        // T is an integer type, thus we can simply use the modulo operator
        return ((value + offset) % alignment) == 0;
    }

    template<typename T>
    inline bool IsAligned(T* value, unsigned int alignment, unsigned int offset)
    {
        // T is a pointer-type, which must first be cast into a suitable integer before we can use the modulo operator
        return ((union_cast<uintptr_t>(value) + offset) % alignment) == 0;
    }
} // namespace internal

X_NAMESPACE_END

#if X_ENABLE_ASSERTIONS

#if X_ENABLE_ASSERTIONS_SOURCE_INFO
#define X_SOURCE_INFO_ASSERT X_SOURCE_INFO
#else
static const X_NAMESPACE(core)::SourceInfo g_blank_source_info("", -1, "", "");
#define X_SOURCE_INFO_ASSERT g_blank_source_info
#endif // !X_ENABLE_ASSERTIONS_SOURCE_INFO

#define X_ASSERT_IMPL_VAR(variable, n)                  .Variable(X_PP_STRINGIZE(variable), variable)
#define X_ASSERT_IMPL_VARS(...)							X_PP_EXPAND_ARGS(X_ASSERT_IMPL_VAR, __VA_ARGS__), X_BREAKPOINT)
#define X_ASSERT(condition, format, ...)				(condition) ? X_UNUSED(true) : (X_NAMESPACE(core)::Assert(X_SOURCE_INFO_ASSERT, "%s" format, "Assertion \"" #condition "\" failed. ", __VA_ARGS__) X_ASSERT_IMPL_VARS
#define X_ASSERT_VERIFY(condition, format, ...)			if(!condition) { X_NAMESPACE(core)::Assert(X_SOURCE_INFO_ASSERT, "%s" format, "Assertion \"" #condition "\" failed. ", __VA_ARGS__); return; }
#define X_ASSERT_NOT_NULL(ptr)                          (ptr != nullptr) ? (ptr) : ((X_ASSERT(ptr != nullptr, "Pointer \"" #ptr "\" is null.")()), nullptr)
#define X_ASSERT_UNREACHABLE()                          X_ASSERT(false, "Source code defect, code should never be reached.")()
#define X_ASSERT_NOT_IMPLEMENTED()                      X_ASSERT(false, "This function is not implemented yet.")()
#define X_ASSERT_DEPRECATED()                           X_ASSERT(false, "This function is deprecated.")()
#define X_ASSERT_ALIGNMENT(argument, alignment, offset) X_ASSERT(X_NAMESPACE(core)::internal::IsAligned(argument, alignment, offset), "Argument \"" #argument "\" is not properly aligned.") (argument, alignment, offset)
#else
#define X_ASSERT(condition, format, ...)                X_UNUSED(condition), X_UNUSED(format), X_UNUSED(__VA_ARGS__), X_UNUSED
#define X_ASSERT_VERIFY(condition, format, ...)			if(!condition) { return; } X_UNUSED(format), X_UNUSED(__VA_ARGS__)

#if X_COMPILER_CLANG
#define X_ASSERT_NOT_NULL(ptr) (decltype(ptr)) ptr
#else
#define X_ASSERT_NOT_NULL(ptr) ptr
#endif // !X_COMPILER_CLANG

#define X_ASSERT_UNREACHABLE()
#define X_ASSERT_NOT_IMPLEMENTED()
#define X_ASSERT_DEPRECATED()
#define X_ASSERT_ALIGNMENT(argument, alignment, offset) X_UNUSED(argument), X_UNUSED(alignment), X_UNUSED(offset)
#endif

#endif // X_ASSERTMACROS_H_
