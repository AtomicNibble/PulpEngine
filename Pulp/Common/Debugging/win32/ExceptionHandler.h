#pragma once

#ifndef X_EXCEPTIONHANDLER_H_
#define X_EXCEPTIONHANDLER_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \brief Custom exception handler taking care of any unfiltered exception.
/// \details By installing a custom exception handler, we get the chance to gather a call stack and write a crash dump
/// whenever some unexpected exception is raised. Exceptions in this context are not C++ exceptions, but rather exceptions
/// raised by the OS because of illegal operations such as access violations, floating-point/integer overflows,
/// divisions by zero, stack overflows, etc. In addition to those exceptions, other custom handlers intercept calls to
/// abort(), pure virtual function calls, and function calls with invalid parameters.
///
/// Whenever an exception is caught by the exception handler, it will attempt to output both machine-independent and
/// machine-dependent information, and write a crash minidump for later debugging.
/// \remark Code generation is enabled/disabled via the preprocessor option \ref X_ENABLE_UNHANDLED_EXCEPTION_HANDLER. Disabling
/// symbol resolution reduces the executable's size, and may be useful for some builds.
/// \sa X_ENABLE_UNHANDLED_EXCEPTION_HANDLER abortHandler pureVirtualFunctionCallHandler invalidParameterHandler debugging::WriteMiniDump
namespace exceptionHandler
{
    /// \brief Starts the exception handler.
    void Startup(void);

    /// \brief Shuts down the exception handler.
    void Shutdown(void);
} // namespace exceptionHandler

X_NAMESPACE_END

#endif // X_EXCEPTIONHANDLER_H_
