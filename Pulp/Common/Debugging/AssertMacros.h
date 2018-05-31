#pragma once

#ifndef X_ASSERTMACROS_H_
#define X_ASSERTMACROS_H_

#include "Casts/union_cast.h"

X_NAMESPACE_BEGIN(core)

namespace internal
{
    /// \brief Used internally by \ref X_ASSERT_ALIGNMENT to check whether an integer type is aligned.
    /// \sa X_ASSERT X_ASSERT_ALIGNMENT
    template<typename T>
    inline bool IsAligned(T value, unsigned int alignment, unsigned int offset)
    {
        // T is an integer type, thus we can simply use the modulo operator
        return ((value + offset) % alignment) == 0;
    }

    /// \brief Used internally by \ref X_ASSERT_ALIGNMENT to check whether a pointer type is aligned.
    /// \sa X_ASSERT X_ASSERT_ALIGNMENT
    template<typename T>
    inline bool IsAligned(T* value, unsigned int alignment, unsigned int offset)
    {
        // T is a pointer-type, which must first be cast into a suitable integer before we can use the modulo operator
        return ((union_cast<uintptr_t>(value) + offset) % alignment) == 0;
    }
} // namespace internal

X_NAMESPACE_END

/// \def X_ASSERT_IMPL_VAR
/// \brief Internal macro used by \ref X_ASSERT_IMPL_VARS.
/// \sa X_ASSERT

/// \def X_ASSERT_IMPL_VARS
/// \brief Internal macro used by \ref X_ASSERT.
/// \sa X_ASSERT

/// \def X_ASSERT
/// \ingroup Debugging
/// \brief Improved assertion macro using the custom Assert class.
/// \details This macro allows asserting that a certain condition holds true, and allows outputting a formatted message
/// to the user (along with additional information) as well as a list of variables in case the condition does not hold.
/// It uses parts of the preprocessor library as well as the custom Assert class internally.
///
/// If the given condition holds, nothing will be done and execution continues normally.
///
/// If the given condition is false, a temporary Assert instance will be created, several calls to Assert::Variable()
/// will be done, and a breakpoint will be triggered after the temporary Assert instance goes out-of-scope. This is
/// achieved by using the comma-operator, which ensures that assertion handlers and loggers had a chance to deal with
/// the assertion before a breakpoint is being hit.
///
/// The syntax of the \ref X_ASSERT macro is the following:
/// \code
///   X_ASSERT(condition, message, optional comma-separated list of message parameters)(optional comma-separated list of variables/values);
/// \endcode
/// \remark Note the second pair of parentheses which is used for creating a variable number of calls to Assert::Variable().
///
/// An example of a simple assertion that always fires, and simply outputs a formatted message:
/// \code
///   X_ASSERT(false, "Key %s could not be found.", key)();
/// \endcode
/// \remark Note the empty pair of parentheses at the end.
///
/// A different example, showing how to output a variable's name and value:
/// \code
///   X_ASSERT(from >= std::numeric_limits::min(), "Number to cast exceeds numeric limits.")(from);
/// \endcode
/// \remark Note the second pair of parentheses, which is expanded by the preprocessor into a call to Assert::Variable().
///
/// A more thorough example, combining both the formatted message and a list of variables:
/// \code
///   X_ASSERT(m_start + i < m_end, "Item %d cannot be accessed. Subscript out of range.", i)(m_start, m_end, m_allocEnd);
/// \endcode
/// For a thorough explanation of how the assertion system works internally,
/// see http://www.altdevblogaday.com/2011/10/12/upgrading-assert-using-the-preprocessor/.
/// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_ASSERTIONS. If disabled,
/// a call to \ref X_ASSERT will not generate any instructions, reducing the executable's size and generally improving
/// performance. It is recommended to disable assertions in retail builds.
/// \sa X_ENABLE_ASSERTIONS Assert assertionDispatch logDispatch X_ASSERT_NOT_NULL X_ASSERT_UNREACHABLE X_ASSERT_NOT_IMPLEMENTED X_ASSERT_ALIGNMENT

/// \def X_ASSERT_NOT_NULL
/// \ingroup Debugging
/// \brief Asserts that a given pointer is not null.
/// \details This macro is a convenience macro that asserts that a given pointer is not null, and uses the assertion
/// facility internally.
///
/// The macro can be used both as a single statement, as well as in assignments:
/// \code
///   // single statement, asserts that ptr is not null
///   X_ASSERT_NOT_NULL(ptr);
///
///   // assignment, assigns the value of ptr to otherPtr, while asserting that ptr is not null
///   int* otherPtr = X_ASSERT_NOT_NULL(ptr);
///
///   // assignment works in constructor initialization lists, too
///   MyClass::MyClass(void* ptr)
///     : m_ptr(X_ASSERT_NOT_NULL(ptr))
///   {
///   }
/// \endcode
/// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_ASSERTIONS. If disabled,
/// a call to \ref X_ASSERT_NOT_NULL will not generate any instructions but only return the given pointer, reducing the
/// executable's size and generally improving performance. It is recommended to disable assertions in retail builds.
/// \sa X_ENABLE_ASSERTIONS X_ASSERT X_ASSERT_UNREACHABLE X_ASSERT_NOT_IMPLEMENTED X_ASSERT_ALIGNMENT

/// \def X_ASSERT_UNREACHABLE
/// \ingroup Debugging
/// \brief Asserts that a certain part of the code is never reached.
/// \details This macro is a convenience macro that asserts that a certain piece of code is never reached, and uses the assertion
/// facility internally. It is useful for ensuring that certain conditions can never occur, especially when calling
/// OS or third-party APIs.
/// \code
///   // will always fire whenever the assert is executed
///   X_ASSERT_UNREACHABLE();
/// \endcode
/// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_ASSERTIONS. If disabled,
/// a call to \ref X_ASSERT_UNREACHABLE will not generate any instructions, reducing the executable's size and generally
/// improving performance. It is recommended to disable assertions in retail builds.
/// \sa X_ENABLE_ASSERTIONS X_ASSERT X_ASSERT_NOT_NULL X_ASSERT_NOT_IMPLEMENTED X_ASSERT_ALIGNMENT

/// \def X_ASSERT_NOT_IMPLEMENTED
/// \ingroup Debugging
/// \brief Asserts that a certain part of the code is not implemented yet.
/// \details This macro is a convenience macro that asserts that a certain piece of code has not been implemented yet,
/// and uses the assertion facility internally.
/// \code
///   // will always fire whenever the assert is executed
///   X_ASSERT_NOT_IMPLEMENTED();
/// \endcode
/// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_ASSERTIONS. If disabled,
/// a call to \ref X_ASSERT_NOT_IMPLEMENTED will not generate any instructions, reducing the executable's size and generally
/// improving performance. It is recommended to disable assertions in retail builds.
/// \sa X_ENABLE_ASSERTIONS X_ASSERT X_ASSERT_NOT_NULL X_ASSERT_UNREACHABLE X_ASSERT_ALIGNMENT

/// \def X_ASSERT_ALIGNMENT
/// \ingroup Debugging
/// \brief Asserts that a given argument is properly aligned to a certain boundary.
/// \details This macro is a convenience macro that asserts that a given argument is aligned to the given boundary,
/// and uses the assertion facility internally. The macro forwards the check to an internal method, thus making it
/// work for both integer as well as pointer types.
/// \code
///   // asserts that (ptr+4) is aligned to 32 bytes. ptr can be of any pointer type
///   X_ASSERT_ALIGNMENT(ptr, 32, 4);
///
///   // asserts that (number+8) is aligned to 64 bytes. number can be of any integer type
///   X_ASSERT_ALIGNMENT(number, 64, 8);
/// \endcode
/// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_ASSERTIONS. If disabled,
/// a call to \ref X_ASSERT_ALIGNMENT will not generate any instructions, reducing the executable's size and generally
/// improving performance. It is recommended to disable assertions in retail builds.
/// \sa X_ENABLE_ASSERTIONS X_ASSERT X_ASSERT_NOT_NULL X_ASSERT_UNREACHABLE X_ASSERT_NOT_IMPLEMENTED
#if X_ENABLE_ASSERTIONS
#define X_ASSERT_IMPL_VAR(variable, n)                  .Variable(X_PP_STRINGIZE(variable), variable)
#define X_ASSERT_IMPL_VARS(...)							X_PP_EXPAND_ARGS(X_ASSERT_IMPL_VAR, __VA_ARGS__), X_BREAKPOINT)
#define X_ASSERT(condition, format, ...)				(condition) ? X_UNUSED(true) : (X_NAMESPACE(core)::Assert(X_SOURCE_INFO, "%s" format, "Assertion \"" #condition "\" failed. ", __VA_ARGS__) X_ASSERT_IMPL_VARS
#define X_ASSERT_NOT_NULL(ptr)                          (ptr != nullptr) ? (ptr) : ((X_ASSERT(ptr != nullptr, "Pointer \"" #ptr "\" is null.")()), nullptr)
#define X_ASSERT_UNREACHABLE()                          X_ASSERT(false, "Source code defect, code should never be reached.")()
#define X_ASSERT_NOT_IMPLEMENTED()                      X_ASSERT(false, "This function is not implemented yet.")()
#define X_ASSERT_ALIGNMENT(argument, alignment, offset) X_ASSERT(X_NAMESPACE(core)::internal::IsAligned(argument, alignment, offset), "Argument \"" #argument "\" is not properly aligned.") (argument, alignment, offset)
#else
#define X_ASSERT(condition, format, ...) X_UNUSED(condition), X_UNUSED(format), X_UNUSED(__VA_ARGS__), X_UNUSED

#if X_COMPILER_CLANG
#define X_ASSERT_NOT_NULL(ptr) (decltype(ptr)) ptr
#else
#define X_ASSERT_NOT_NULL(ptr) ptr
#endif // !X_COMPILER_CLANG

#define X_ASSERT_UNREACHABLE()
#define X_ASSERT_NOT_IMPLEMENTED()
#define X_ASSERT_ALIGNMENT(argument, alignment, offset) X_UNUSED(argument), X_UNUSED(alignment), X_UNUSED(offset)
#endif

#endif // X_ASSERTMACROS_H_
