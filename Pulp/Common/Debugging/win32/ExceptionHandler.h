#pragma once

#ifndef X_EXCEPTIONHANDLER_H_
#define X_EXCEPTIONHANDLER_H_

X_NAMESPACE_BEGIN(core)


// Custom exception handler taking care of any unfiltered exception.
// By installing a custom exception handler, we get the chance to gather a call stack and write a crash dump
// whenever some unexpected exception is raised. Exceptions in this context are not C++ exceptions, but rather exceptions
// raised by the OS because of illegal operations such as access violations, floating-point/integer overflows,
// divisions by zero, stack overflows, etc. In addition to those exceptions, other custom handlers intercept calls to
// abort(), pure virtual function calls, and function calls with invalid parameters.
namespace exceptionHandler
{
    void Startup(void);
    void Shutdown(void);
} // namespace exceptionHandler

X_NAMESPACE_END

#endif // X_EXCEPTIONHANDLER_H_
